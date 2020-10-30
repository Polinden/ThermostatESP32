#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"
#include "Adafruit_SHT31.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SPI.h>
#include "GyverTimer.h"

#define SCREEN_WIDTH 128 
#define SCREEN_HEIGHT 64 
#define OLED_MOSI  25
#define OLED_CLK   17
#define OLED_DC    27
#define OLED_CS    16
#define OLED_RESET 14
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);


const int ledPin = 2;
const char* ssid = "***";
const char* password = "***";
IPAddress ip(192,168,77,225); 
IPAddress gateway(192,168,77,1);
IPAddress subnet(255,255,255,0);
IPAddress primaryDNS(8, 8, 8, 8); 
IPAddress secondaryDNS(192.168.77.1); 

Adafruit_SHT31 sht31 = Adafruit_SHT31();
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
GTimer myTimer1(MS, 2000);
GTimer myTimer2(MS, 4000);

boolean is_on=false;
boolean start_use=false;
boolean stop_use=false;
float start_t=20;
float stop_t=30;
float t=0.0;
float h=0.0;

String processor(const String& var){
  String response;
   if (var == "start_t") {
      response=String(start_t);
      return response;
    };
   if (var == "stop_t") {
      response=String(stop_t);
      return response;
    };
    if (var == "start_use") {
      if (start_use) return String("true");
      else return String("false");
      return response;
    };
   if (var == "stop_use") {
      if (stop_use) return String("true");
      else return String("false");
    };
  return String();
}


void withAuto(float t) {
   if ((start_use) && (t<start_t)) {digitalWrite(ledPin, HIGH);  boolean is_on=true;}
   if ((stop_use) && (t>stop_t))  {digitalWrite(ledPin, LOW); boolean is_on=false;}  
}

void withScreen(float t, float h){
   display.clearDisplay();
   display.setTextSize(2);             
   display.setTextColor(SSD1306_WHITE);
   display.setCursor(0,0);             
   display.println("My spiders"); 
   display.display();
   display.drawLine(0, 22, display.width(), 22, SSD1306_WHITE);
   display.setCursor(0,34);             
   display.setTextSize(1.7);                
   display.println("Temp: "+String(t)+"C");  
   display.println("");  
   display.println("Humid: "+String(h)+"%");  
   display.display();    
}

void setup(){
  Serial.begin(115200);
  pinMode(ledPin, OUTPUT);

  if(!SPIFFS.begin(true)){
   Serial.println("An Error has occurred while mounting SPIFFS");
  }  
  
  WiFi.disconnect(true);
  if (!WiFi.config(ip, gateway, subnet, primaryDNS, secondaryDNS)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);

  Serial.println("SHT31 test");
  if (! sht31.begin(0x44)) {  
    Serial.println("Couldn't find SHT31");
  }


  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/ind.html", String(), false, processor);
  });


  server.on("/on", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, HIGH);   
    boolean is_on=true;
    request->send(SPIFFS, "/ind.html", String(), false, processor);
  });

  server.on("/off", HTTP_GET, [](AsyncWebServerRequest *request){
    digitalWrite(ledPin, LOW); 
    boolean is_on=false;
    request->send(SPIFFS, "/ind.html", String(), false, processor);
  });

  server.on("/go_auto", HTTP_GET, [](AsyncWebServerRequest *request){
     if (request->hasParam("start_use")) {      
        AsyncWebParameter* p1 = request->getParam("start_t"); 
        start_t=atof(p1->value().c_str()); 
        start_use=true;
     } else start_use=false;
     if (request->hasParam("stop_use")) {      
        AsyncWebParameter* p2 = request->getParam("stop_t"); 
        stop_t=atof(p2->value().c_str());
        stop_use=true;
     } else stop_use=false;
    request->redirect("/");   
  });

  server.addHandler(&ws);
  server.begin();

  if(!display.begin(SSD1306_SWITCHCAPVCC)) Serial.println(F("SSD1306 allocation failed"));
  display.display();
  delay(1000); 
  display.clearDisplay();

}
 

void loop(){
  if (myTimer1.isReady()) {
        t=sht31.readTemperature();
        h=sht31.readHumidity();
        if (!isnan(t) && !isnan(h)) {withAuto(t); withScreen(t, h);
  }
  if (myTimer2.isReady()){
     if (!isnan(t) && !isnan(h)) {        
        String temp= String(t)+"+++"+String(h)+"+++";
        if(is_on) temp=temp+"включен"; else temp=temp+"выключен";
        ws.textAll(temp.c_str());
     }   
  }
}
