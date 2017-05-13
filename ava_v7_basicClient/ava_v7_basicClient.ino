#include "Adafruit_NeoPixel.h"
#include "Adafruit_GFX.h"
#include "Adafruit_SSD1306.h"
#include "Wire.h"
#include "RtcDS1307.h"
#include "ESP8266WiFi.h"
#include "Timer.h" 

#define PIX_PIN D3  
#define MID_PIN D0 // interrupt
#define LEF_PIN D1
#define RIG_PIN D2
#define TON_PIN D4
#define OLED_RESET D7 // Not Connected, software just requires it

#define PIXEL_COUNT 12

// objects
RtcDS1307<TwoWire> Rtc(Wire);
Adafruit_SSD1306 display(OLED_RESET);
Adafruit_NeoPixel strip = Adafruit_NeoPixel(PIXEL_COUNT, PIX_PIN, NEO_GRB + NEO_KHZ800);
Timer t;    


// global constants
const String months[] = {"Dezembr","Jan","Feb","Mar","Apr","Mai","Jun","Jul","Okt","Nov","Dez"}; // dunno have to try out
const char* ssid     = "AVATIFY - Motivation Platform"; // later store a list of wifi passwords
const char* password = "h4ckingR00m";

const char* host = "avatify.mitmachine.de";
const char* user = "kennyawesome";
const int httpPort = 80;

// global variables, volatile if interrupts can change value
//int anzeige[] = {6,0,5,1,4,2,3}; // -> ascii-Table
volatile int midPressed = 0;
volatile int lefPressed = 0;
volatile int rigPressed = 0;
volatile int timeCount = 0;
volatile int menuIndex[] = {0,0}; // list of menus, can update
volatile long formerSum = 1; // first show clock

void setup() {
  WiFi.forceSleepBegin();
  Wire.begin(D5,D6);
  // put your setup code here, to run once:
  Serial.begin(57600);
  strip.begin();
  strip.setBrightness(20); // should be stored in EEPROM
  strip.show();
  
  Rtc.Begin();
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);

  pinMode(MID_PIN, INPUT_PULLUP);
  pinMode(LEF_PIN, INPUT_PULLUP);
  pinMode(RIG_PIN, INPUT_PULLUP);
  pinMode(TON_PIN, OUTPUT);
  digitalWrite(TON_PIN,HIGH);

  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3c);
  display.clearDisplay();
  display.display();

  //attachInterrupt(RIG_PIN, toggle, CHANGE);
  t.every(200, takeReading);
  t.every(200, menu);
  t.every(200, view);
  t.every(60000, scheduler); // every minute check, 10 min
}

void loop(){
  t.update();
}



// menü -> eigener prozess
  // uhrenanzeige -> overlap
  // menutree -> anzeige
  // marquee anzeige
  // atemübung
  // befindlichkeit -> how to note on 
  // config
    // wifi config
    // time to display
    // brightness of led
  // Doubleclick
// todo
  // ntc update
  // timeout -> schedule keeper (list, and secondwise?) -> store time efficiently
  // Load images
  // images + text
  // fileIO

void scheduler(){
  
}

void view(){
    
}
  

void menu(){
  if(menuIndex[0] == 0){
    if(0 != (midPressed + lefPressed + rigPressed)){
      timeCount = 0;
      // here starts the menu
      showTime();
      resetInput();
      menuIndex[0] = 1;
    } 
  }else if(menuIndex[0] == 1){
      if(timeCount > 15){
        saveEnergy();
        menuIndex[0] = 0;
        menuIndex[1] = 0;
      } else {
        if(0 != (midPressed + lefPressed + rigPressed)){
          timeCount = 0;
          resetInput();
          //show current task, if showed, then the real menu
          testdrawSmily();
          showDistribution(7, 3, 2);
        }
      }
   }
}

void resetInput(){
  midPressed = 0;
  lefPressed = 0;
  rigPressed = 0;
}

void showTime(){
  RtcDateTime now = Rtc.GetDateTime();
  String padHour = "";
  String padMinute = "";
  if(now.Hour() < 10){
    padHour = "0";
  }
  if(now.Minute() < 10){
    padMinute = "0";
  }
  String firstLine = padHour + String(now.Hour()) + ":" + padMinute +String(now.Minute());
  String secondLine = String(now.Day()) + "ter " + months[now.Month()];

  display.clearDisplay();
   
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println(firstLine);
  display.println(secondLine);
  display.display();

  strip.setPixelColor(now.Hour()%12, 0x0000ff); // cyan
  strip.setPixelColor(int(now.Minute()/5), 0xff0000); // cyan
  strip.show();

  
}

void showAvatar(){
  showDistribution(7, 3, 2);
  //testdrawSmily();
  showTime();
}

void saveEnergy(){
  strip.clear();
  strip.show();

  display.clearDisplay();
  display.display();

  WiFi.forceSleepBegin();

  // should detach timer interrupt?
}

void updateData(String action, String key, String value){
  WiFi.forceSleepWake();
  WiFi.begin(ssid, password);

  int connectionTry = 0;
  while ((WiFi.status() != WL_CONNECTED) || connectionTry < 5)  {
    delay(500);
    connectionTry++;
    Serial.print(".");
  }

  if(WiFi.status() == WL_CONNECTED){
    Serial.println(WiFi.localIP());
    WiFiClient client;
    
    if (client.connect(host, httpPort)) {
      String url = "/api.php";
      url += "?user=";
      url += user;
      url += "&action=";
      url += action;
      url += "&key=";
      url += key;
      url += "&value=";
      url += value;
      
      client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" + 
               "Connection: close\r\n\r\n");
      unsigned long timeout = millis();
      while ((client.available() == 0) || ((millis() - timeout) > 5000)){}
      if(millis() - timeout < 5000){
        // for single line result
        //display.drawProgressBar(0, 32, 120, 10, progress);
        int counter = 0;
        String result;
        // Read all the lines of the reply from server and print them to Serial
        while(client.available()){
          String line = client.readStringUntil('\r');
          if(counter == 9){
            if(line.length() >= 10){
              result = line;
            }
            Serial.print(line);
          }
          counter++;
        }
      }else{
        Serial.println("Client connection failed");
      }
    }else{
      Serial.println("Host connection failed");
    } 
  }
  WiFi.forceSleepBegin();
}

void takeReading(){
  if(!digitalRead(MID_PIN)){
    midPressed++;
    pinMode(MID_PIN, OUTPUT);
    digitalWrite(MID_PIN,HIGH);
    pinMode(MID_PIN, INPUT_PULLUP);
  }
  if(!digitalRead(LEF_PIN)){
    lefPressed++;
  }
  if(!digitalRead(RIG_PIN)){
    rigPressed++;
  }
  timeCount++;
}

void updateText(){
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("ALOHA");
  display.display();
}

void showDistribution(int important, int longterm,int creative){
  
  for(int i = 0; i<important; i++){
    strip.setPixelColor(i, 0xff0266); // magenta
  }
  for(int i = important; i<important+creative; i++){
    strip.setPixelColor(i, 0xffb200); // orange
  }
  for(int i = important+creative; i<important+longterm+creative; i++){
    strip.setPixelColor(i, 0x0efec8); // cyan
  }
  strip.show();
}

void testdrawSmily() {
 display.clearDisplay();
 display.drawCircle(display.width()/2 - 30 , display.height()/2, 15, WHITE);
 display.drawCircle(display.width()/2 + 30 , display.height()/2, 15, WHITE);
 display.fillCircle(display.width()/2 - 30, display.height()/2, 5, WHITE);
 display.fillCircle(display.width()/2 + 30, display.height()/2, 5, WHITE);
 display.display();
}
