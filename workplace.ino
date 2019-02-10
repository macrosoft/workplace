#include <ChainableLED.h>
#include <ESP8266WiFi.h>
#include "config.h"

#define CLK_LED_PIN D0
#define DATA_LED_PIN D1

ChainableLED led(CLK_LED_PIN, DATA_LED_PIN, 1);

void setup() {
  led.init();
  led.setColorRGB(0, random(255), random(255), random(255));
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
}
