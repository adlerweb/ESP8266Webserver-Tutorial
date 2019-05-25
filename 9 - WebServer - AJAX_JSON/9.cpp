#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ESP8266WebServer.h>
#include <FS.h>


#define SSID      "ADLERWEB_ESPTEST"
#define PASSWD    "ESP8266-Testsystem-2"
#define HOSTNAME  "mosquitoled"

ESP8266WebServer server ( 80 );

const String HTML = R"DATA(
<!doctype html>
<html lang="de">
    <head>
        <meta charset="utf-8"/>
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>Test als HTML</title>
    </head>

    <body>
        <h1>Test als HTML</h1>
        <p>Dies ist ein Test mit HTML</p>
    </body>
</html>
)DATA";

void httpIndex() {
  server.send(200, "text/plain", "Hello World!");
}

void httpHTML() {
  server.send(200, "text/html", HTML);
}

void httpTime() {
  String output = "I am running for ";
  output += String(millis());
  output += " milliseconds.";
  server.send(200, "text/plain", output);
}

void httpTimeJson() {
  String output = "{ \"uptime\":";
  output += String(millis());
  output += " }";
  server.send(200, "application/json", output);
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

  if(!SPIFFS.begin()) {
    Serial.println("SPIFFS Mount failed");
    while(true) yield(); //Stop here
  }

  server.on("/", httpIndex);
  server.on("/time", httpTime);
  server.on("/html", httpHTML);

  server.serveStatic("/fs", SPIFFS, "/fs.html");
  server.serveStatic("/biba.png", SPIFFS, "/biba.png");

  server.serveStatic("/time.html", SPIFFS, "/time.html");
  server.on("/time.json", httpTimeJson);

  server.onNotFound(httpNotFound);
  
  server.begin();
  Serial.println ( "HTTP server started" );
}

void loop() {
  MDNS.update();
  server.handleClient();
}