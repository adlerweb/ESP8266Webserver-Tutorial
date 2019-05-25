#include <Arduino.h>
#include <ArduinoOTA.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>

#define ssid      "freifunk-myk.de"
#define password  ""

#define HOSTNAME "esphttptest"

ESP8266WebServer server ( 80 );

byte party = 0;
byte led = 0;

byte party_anim = 0;

const float _ledFactor = (100 * log10(2))/(log10(1024));

void sendState() {
  String json = "{\"led\":\"" + String(led) + "\",";
  json += "\"party\":\"" + String(party) + "\"}";

  server.send(200, "application/json", json);

  Serial.print(millis());
  Serial.print(" [STATE] LED:");
  Serial.print(led);
  Serial.print(" - Party:");
  Serial.print(party);
  Serial.print(" - PA:");
  Serial.println(party_anim);
}

void updateParty() {
  String gpio = server.arg("id");
  String state = server.arg("state");
  String success = "1";

  Serial.print(millis());
  Serial.print(" [PARTY] ID: ");
  Serial.print(gpio);
  Serial.print(" - RState:");
  Serial.print(state);

  party = constrain(state.toInt(), 0, 2); 

  Serial.print(" - SState:");
  Serial.println(party);
  
  String json = "{\"state\":\"" + String(state) + "\",";
  json += "\"success\":\"" + String(success) + "\"}";
    
  server.send(200, "application/json", json);
}

void updateGpio() {
  String gpio = server.arg("id");
  String state = server.arg("state");
  String success = "1";
  byte newstate = 0;

  Serial.print(millis());
  Serial.print(" [GPIO] ID: ");
  Serial.print(gpio);
  Serial.print(" - RState:");
  Serial.print(state);

  newstate = constrain(state.toInt(), 0, 100); 

  Serial.print(" - SState:");
  Serial.print(newstate);
  
  if ( gpio == "LED" ) {
    unsigned int ledout = pow(2, (newstate / _ledFactor)) - 1;
    if(newstate == 100) {
      digitalWrite(LED_BUILTIN, LOW);
    }else if(newstate == 0) {
      digitalWrite(LED_BUILTIN, HIGH);
    }else{
      analogWrite(LED_BUILTIN, 1024-ledout);
    }
    led = newstate;
    party = 0;
    Serial.print(" - LED@");
    Serial.print(255-ledout);
  } else {
    success = "0";
  }
  
  String json = "{\"gpio\":\"" + String(gpio) + "\",";
  json += "\"state\":\"" + String(state) + "\",";
  json += "\"success\":\"" + String(success) + "\"}";

  Serial.print(" - OK? ");
  Serial.print(success);
    
  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);

  delay(2000);

  Serial.println("\nBoot");

  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, HIGH);

  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");
    while(true) yield(); //Stop here
  }

  Serial.print("Connecting.");
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(ssid, password);

  do{
    delay(500);
    Serial.print(".");
  } while (WiFi.status() != WL_CONNECTED);

  Serial.println("\nWiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  ArduinoOTA.setHostname(HOSTNAME);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nOTA End");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("OTA Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("OTA ready");

  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");
    while(true) yield(); //Stop here
  }

  server.on("/gpio", updateGpio);
  server.on("/party", updateParty);
  server.on("/state.json", sendState);

  server.serveStatic("/", SPIFFS, "/index.html");
  server.serveStatic("/logo.png", SPIFFS, "/logo.png");

  server.begin();
  Serial.println ( "HTTP server started" );

  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Error setting up MDNS responder!");
  }
}

unsigned long nexttime = 0;

void loop() {
  unsigned int dly = 0;
  unsigned int out = 0;

  if(party > 0) {
    switch(party) {
      case 1: //Ring
        party_anim++;
        if(party_anim >= 2) party_anim = 0;
        switch(party_anim) {
          case 0:
            digitalWrite(LED_BUILTIN, HIGH);
          break;
          case 1:
            digitalWrite(LED_BUILTIN, LOW);
          break;
        }
        dly = 500;
      break;
      case 2: //Fade

        if(party_anim >= 200) {
          party_anim = 0;
        }else if(party_anim > 100) {
          out = pow(2, ((100 - (party_anim - 100)) / _ledFactor)) - 1;
        }else {
          out = pow(2, (party_anim / _ledFactor)) - 1;
        }

        analogWrite(LED_BUILTIN, 1024-out);
        Serial.println(1024-out);

        party_anim++;

        dly = 15;
      break;
    }
  }
  
  unsigned long target = millis() + dly;
  if(dly>0) {
    Serial.print("Sleeping for ");
    Serial.print(dly);
    Serial.print(" ms...");
  }
  do {
    yield();
    ArduinoOTA.handle();
    server.handleClient();
  } while(millis() <= target);
  if(dly>0) Serial.println("done");
  dly = 0;

  if(nexttime <= millis()) {
    Serial.print(millis());
    Serial.println(" - Still alive");
    nexttime=millis() + 2000;
  }
}
