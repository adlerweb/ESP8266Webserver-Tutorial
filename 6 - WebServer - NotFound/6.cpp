#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>


#define SSID      "ADLERWEB_ESPTEST"
#define PASSWD    "ESP8266-Testsystem-2"
#define HOSTNAME  "mosquitoled"

ESP8266WebServer server ( 80 );

void httpIndex() {
  server.send(200, "text/plain", "Hello World!");
}

void httpTime() {
  String output = "I am running for ";
  output += String(millis());
  output += " milliseconds.";
  server.send(200, "text/plain", output);
}

void httpNotFound() {
  server.send(404, "text/plain", "404: Not found");
}

void setup() {
  Serial.begin(115200);

  delay(500);

  Serial.print("Connecting.");
  WiFi.mode(WIFI_STA);
  WiFi.hostname(HOSTNAME);
  WiFi.begin(SSID, PASSWD);
  do{
    delay(500);
    Serial.print(".");
  } while (WiFi.status() != WL_CONNECTED);
  Serial.print("OK\nIP: ");
  Serial.println(WiFi.localIP());

  if (!MDNS.begin(HOSTNAME)) {
    Serial.println("Error setting up MDNS responder!");
  }

  server.on("/", httpIndex);
  server.on("/time", httpTime);
  server.onNotFound(httpNotFound);
  
  server.begin();
  Serial.println ( "HTTP server started" );
}

void loop() {
  MDNS.update();
  server.handleClient();
}