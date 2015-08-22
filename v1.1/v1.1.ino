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
int tarBrightness = 100;
byte curBrightness = 0;
byte zcState = 0; // 0 = ready; 1 = processing; 2 = double trigger fix

byte bIsGet = 0;
int smoothTime = 3000;
unsigned long previousMillis;
unsigned long currentMillis;
int iMillis;
byte oldBrightness = 0;
byte newBrightness = 0;
int iContrast = 0;

byte bIsOn = 0;
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

  server.on("/", []() {
    if (server.arg("s") != "") {
      if (server.arg("s") == "1")
      {
        /*
        iContrast = (int)tarBrightness - (int)0;
        newBrightness = tarBrightness;
        previousMillis = millis();

        bIsGet = 1;
        */
        bIsOn = 1;
      }

      if (server.arg("s") == "0")
      {
        bIsGet = 0;
        bIsOn = 0;
      }


      /*
      if (server.arg("s") == "1" || server.arg("s") == "on" || server.arg("s") == "true") {
        state = 1;
      }
      else if (server.arg("s") == "t") {
        state = !state;
      }
      else {
        state = 0;
      }
      */
    }

    if (server.arg("b") != "") {
      oldBrightness = newBrightness;
      newBrightness = (byte) server.arg("b").toInt();
      iContrast = (int)newBrightness - (int)oldBrightness;
      previousMillis = millis();
      bIsGet = 1;
    }

    if (server.arg("t") != "") {

      smoothTime = (int) server.arg("t").toInt();


    }

    if (server.arg("f") != "") {
      if (server.arg("f") == "1" || server.arg("f") == "on" || server.arg("f") == "true") {
        fade = 1;
      }
      else {
        fade = 0;
      }
    }

    String s = "{\n   \"s\":";
    s += bIsOn;
    s += ",\n   \"b\":";
    s += tarBrightness;
    s += ",\n   \"f\":";
    s += fade;
    s += ",\n   \"Time\":";
    s += previousMillis;
    s += ",\n   \"smoothTime\":";
    s += smoothTime;
    s += ",\n   \"Contrast\":";
    s += iContrast;
    s += ",\n   \"oldBrightness\":";
    s += oldBrightness;
    s += ",\n   \"newBrightness\":";
    s += newBrightness;
    s += "\n}";

    server.send(200, "text/plain", s);
  });

  server.begin();
  Serial.println("HTTP server started");

  //attachInterrupt(switchPin, switchDetect, CHANGE);
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

  if (bIsOn == 1)
  {
    if (bIsGet == 1)
    {
      currentMillis = millis();

      iMillis = (currentMillis - previousMillis);
      tarBrightness = (byte)(oldBrightness + (int)floor((iMillis / (float)smoothTime) * iContrast));
      /*
      Serial.print(oldBrightness+" o: ");
      Serial.print(iMillis+" i: ");
      Serial.print((int)floor((iMillis / (float)smoothTime) * iContrast)+" c: ");
      */
      //sSerial.println(tarBrightness);
      //Serial.println(((int)(currentMillis - previousMillis)));
      if (currentMillis - previousMillis >= smoothTime)
      {

        tarBrightness = newBrightness;
        bIsGet = 0;
      }
    }
  }

  if (tarBrightness < 128 && tarBrightness > 1) {
    int x;
    if (bIsOn == 1)
    {
      x = map(tarBrightness, 100, 0, 1, 128);

    } else {
      x = map(0, 100, 0, 1, 128);
    }

    int dimtime = (75 * x);  // For 60Hz =>65
    // Serial.println(x);
    delayMicroseconds(dimtime);    // Wait till firing the TRIAC
    digitalWrite(outPin, HIGH);   // Fire the TRIAC
    delayMicroseconds(5);         // triac On propogation delay (for 60Hz use 8.33)
    digitalWrite(outPin, LOW);

  }

}
