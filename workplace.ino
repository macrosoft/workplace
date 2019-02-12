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

float v = 1.0;
float saturation = 1.0;
float offsetHue = 0.0;
byte light = 0;

void setup() {
  led.init();
  led.setColorRGB(0, 0, 0, 0);
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
  webServer.on("/ajax", handleAjax);
  offsetHue = random(1024)/1023.0;
}

void loop() {
  light = map(analogRead(A0), 0, 1023, 100, 0);
  if (light > 40)
    v = 0.0;
  else if (light < 30)
    v = 1.0;
  updateLedColor();
  webServer.handleClient();
}

void updateLedColor() {
  float hue = millis()%10800000/10800000.0 + offsetHue;
  if (hue >= 1.0)
    hue -= 1.0;
  float scaledHue = hue < 3.0/6.0? hue*2.0/3.0:
                    hue < 4.0/6.0? 1.0/3.0 + (hue - 3.0/6.0):
                    hue < 5.0/6.0? 1.0/2.0 + (hue - 4.0/6.0)*2.0:
                    hue;
  byte i = scaledHue*6;
  float f = scaledHue*6 - i;
  float p = v*(1 - saturation);
  float q = v*(1 - f*saturation);
  float t = v*(1 - (1 - f)*saturation);
  float r, g, b;
  switch(i%6){
        case 0: r = v, g = t, b = p; break;
        case 1: r = q, g = v, b = p; break;
        case 2: r = p, g = v, b = t; break;
        case 3: r = p, g = q, b = v; break;
        case 4: r = t, g = p, b = v; break;
        case 5: r = v, g = p, b = q; break;
  }
  led.setColorRGB(0, round(255*r), round(255*g), round(255*b));
}

void handleAjax() {
  webServer.send(200, "text/plain",
                 "{ \"light\": " + String(light) +
                 "}");
}
