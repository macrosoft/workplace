#include <ChainableLED.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "config.h"

#define CLK_LED_PIN D0
#define DATA_LED_PIN D1
#define ONE_WIRE_PIN D4

ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;
ChainableLED led(CLK_LED_PIN, DATA_LED_PIN, 1);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature temp_sensor(&oneWire);
DeviceAddress temp_addr;

float v = 0.0;
byte value = 0;
float saturation = 1.0;
float offsetHue = 0.0;
byte light = 0;
unsigned long oneHzTimer = 0;
unsigned long lightTimer = 0;

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
  if (SPIFFS.begin()) {
    Serial.println("SPIFFS Initialize....ok");
  } else {
    Serial.println("SPIFFS Initialization...failed");
  }
  httpUpdater.setup(&webServer, "/update", USERNAME, PASSWORD);
  webServer.begin();
  webServer.on("/", handleRoot);
  webServer.on("/ajax", handleAjax);
  webServer.on("/jquery.min.js", []() {
    sendFile("/jquery.min.js", "text/javascript");
  });
  offsetHue = random(1024)/1023.0;
  temp_sensor.begin();
  temp_sensor.getAddress(temp_addr, 0);
}

void loop() {
  light = map(analogRead(A0), 0, 1023, 100, 0);
  if (isTimer(lightTimer, 15)) {
    lightTimer = millis();
    if (light > 40 and value > 0)
      --value;
    else if (light < 30 and value < 255)
      ++value;
    v = value/255.0;
  }
  updateLedColor();
  if (isTimer(oneHzTimer, 1000)) {
    oneHzTimer = millis();
    temp_sensor.requestTemperatures();
  }
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

void handleRoot() {
  String content = "<!DOCTYPE html>";
  content += "<html>";
  content += "<head><meta charset=\"utf-8\">";
  content += "<title>Workplace</title>";
  content += "<script src=\"jquery.min.js\"></script>";
  content += "<script type=\"text/javascript\">";
  content += "$(document).ready(function(){";
  content += "$.ajaxSetup({ cache: false });";
  content += "setInterval(function() {";
  content += "$.get(\"ajax\", function(result){";
  content += "vars = JSON.parse(result.trim());";
  content += "$('#light').text(vars.light);";
  content += "$('#temp').text(vars.temp);";
  content += "});},1000);";
  content += "});";
  content += "</script>";
  content += "</head>";
  content += "<body>";
  content += "Light: <span id='light'>-</span><br>";
  content += "Temperature: <span id='temp'>-</span><br>";
  content += "</body>";
  content += "</html>";
  webServer.send(200, "text/html", content);
}

void handleAjax() {
  webServer.send(200, "text/plain",
                 "{ \"light\": " + String(light) +
                 ", \"temp\": " + String(temp_sensor.getTempC(temp_addr),1) +
                 "}");
}

void sendFile(String fileName, String type) {
  File f = SPIFFS.open(fileName, "r");
  if (!f) {
    Serial.println(fileName + " open failed");
  } else {
    webServer.streamFile(f, type);
    f.close();
  }
}

bool isTimer(unsigned long startTime, unsigned long period) {
  unsigned long currentTime;
  currentTime = millis();
  if (currentTime >= startTime) {
    return (currentTime >= (startTime + period));
  } else {
    return (currentTime >= (0xFFFFFFFF - startTime + period));
  }
}
