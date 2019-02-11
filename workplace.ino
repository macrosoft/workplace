#include <ChainableLED.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include "config.h"

#define CLK_LED_PIN D0
#define DATA_LED_PIN D1

ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;
ChainableLED led(CLK_LED_PIN, DATA_LED_PIN, 1);

void setup() {
  led.init();
  led.setColorRGB(0, 255, 64, 0);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("\r\nConnect to ");
  Serial.print(WIFI_SSID);
  for (byte i = 0; i < 80; i++) {
    if (WiFi.status() == WL_CONNECTED)
      break;
    delay(250);
    Serial.print(".");
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("[OK]");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[FAILED]");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("workplace", PASSWORD);
    Serial.print("AP IP address: ");
    Serial.println(WiFi.softAPIP());
  }
  httpUpdater.setup(&webServer, "/update", USERNAME, PASSWORD);
  webServer.begin();
}

void loop() {
  webServer.handleClient();
}
