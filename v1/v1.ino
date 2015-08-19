#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
 
const char* ssid = "NAT.11";
const char* password = "guestnetwork";

const byte switchPin = 14;
const byte zcPin = 5;
const byte outPin = 4;

byte fade = 0;
byte state = 0;
byte tarBrightness = 255;
byte curBrightness = 0;
byte zcState = 0; // 0 = ready; 1 = processing; 2 = double trigger fix

ESP8266WebServer server(80);

void setup(void)
{
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(zcPin, INPUT_PULLUP);
  pinMode(outPin, OUTPUT);

  digitalWrite(outPin, 0);
  
  Serial.begin(115200);
  Serial.println("");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("\nConnected: ");
  Serial.println(WiFi.localIP());
 
  server.on("/", [](){
    if (server.arg("s") != "") {
      if (server.arg("s") == "1" || server.arg("s") == "on" || server.arg("s") == "true") {
        state = 1;
      }
      else if (server.arg("s") == "t") {
        state = !state;
      }
      else {
        state = 0;
      }
    }
    
    if (server.arg("b") != "") {
      tarBrightness = (byte) server.arg("b").toInt();
    }
    
    if (server.arg("f") != "") {
      if (server.arg("f") == "1" || server.arg("f") == "on" || server.arg("f") == "true"){
        fade = 1;
      }
      else {
        fade = 0;
      }
    }
    
    String s = "{\n   \"s\":";
    s += state;
    s += ",\n   \"b\":";
    s += tarBrightness;
    s += ",\n   \"f\":";
    s += fade;
    s += "\n}";
    
    server.send(200, "text/plain", s);
  });
  
  server.begin();
  Serial.println("HTTP server started");
  
  attachInterrupt(switchPin, switchDetect, CHANGE);
  attachInterrupt(zcPin, zcDetect, RISING);
}

void loop(void)
{
  server.handleClient();
}

void switchDetect()
{
  detachInterrupt(switchPin);

  state = !state;
  
  // debounce
  delayMicroseconds(500); // delay(5) causes wdt reset (bug?)
  
  attachInterrupt(switchPin, switchDetect, CHANGE);
}

void zcDetect()
{
  //detachInterrupt(zcPin);
  if (zcState == 0) {
    zcState = 1;
  
    if (curBrightness < 128 && curBrightness > 0) {
      int dimtime = (75*curBrightness);    // For 60Hz =>65    
  delayMicroseconds(dimtime);    // Wait till firing the TRIAC
  digitalWrite(outPin, HIGH);   // Fire the TRIAC
  delayMicroseconds(5);         // triac On propogation delay (for 60Hz use 8.33)
  digitalWrite(outPin, LOW);
//      digitalWrite(outPin, 1);
//      
//      int dimDelay = 75*curBrightness;    
//      delayMicroseconds(dimDelay);
//      
//      digitalWrite(outPin, 0);
//      //delayMicroseconds(150);
    }
    
    if (fade == 1 && (curBrightness > tarBrightness || (state == 0 && curBrightness > 128))) {
      curBrightness -= 1;
    }
    else if (fade == 1 && curBrightness < tarBrightness && state == 1 && curBrightness < 20) {
      curBrightness += 1;
    }
    else if (fade == 0 && state == 1) {
      curBrightness = tarBrightness;
    }
    else if (fade == 0 && state == 0) {
      curBrightness = 0;
    }

    zcState = 2;
  }
  else if (zcState == 2) {
    zcState = 0;
  }
  //attachInterrupt(zcPin, zcDetect, RISING); 
}
