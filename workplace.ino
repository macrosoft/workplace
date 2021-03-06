#include <ChainableLED.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <FS.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <ArduinoOTA.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "config.h"
#include "az_timer.h"
#include "big_digits.h"
#include "ntp.h"

#define CLK_LED_PIN D0
#define DATA_LED_PIN D1
#define SDA_PIN D2
#define SCL_PIN D3
#define ONE_WIRE_PIN D4
WiFiClientSecure secureClient;
HTTPClient http;

ESP8266WebServer webServer(80);
ESP8266HTTPUpdateServer httpUpdater;
ChainableLED led(CLK_LED_PIN, DATA_LED_PIN, 1);

OneWire oneWire(ONE_WIRE_PIN);
DallasTemperature temp_sensor(&oneWire);
DeviceAddress temp_addr;

LiquidCrystal_I2C lcd(0x3F, 16, 2);
BigDigitsPrinter printer(&lcd);
NtpClient ntp;

float v = 0.0;
byte value = 0;
float saturation = 1.0;
float offsetHue = 0.0;
byte light = 0;
AZTimer oneHzTimer(1000);
AZTimer lightTimer(20);
AZTimer updateTimer(250);
int outdoor_temp = -100;
int outdoor_humidity = -100;

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
  temp_sensor.setWaitForConversion(false);
  Wire.begin(SDA_PIN, SCL_PIN);
  lcd.begin();
  lcd.clear();
  lcd.backlight();
  printer.begin();
  secureClient.setInsecure();
  http.begin(secureClient, "https://api.weather.yandex.ru/v1/informers?lat=" + String(LATITUDE) + "&lon=" + String(LONGITUDE));
  http.addHeader("X-Yandex-API-Key", YANDEX_API_KEY);
  ArduinoOTA.setHostname("workplace");
  if (!WiFi.status() == WL_CONNECTED)
    ArduinoOTA.setPassword(PASSWORD);
  ArduinoOTA.onStart([]() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Updating...");
    lcd.blink();
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    lcd.noBlink();
    if (updateTimer.check()) {
      lcd.setCursor(0, 1);
      lcd.print("Progress:");
      lcd.print(progress/(total/100));
      lcd.print("%");
    }
  });
  ArduinoOTA.onEnd([]() {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Rebooting... ");
    lcd.setCursor(12, 0);
  });
  ArduinoOTA.begin();
}

void loop() {
  light = map(analogRead(A0), 0, 1023, 100, 0);
  if (lightTimer.check()) {
    if (light > 40 and value > 0)
      --value;
    else if (light < 30 and value < 255)
      ++value;
    v = value/255.0;
  }
  updateLedColor();
  if (oneHzTimer.check()) {
    temp_sensor.requestTemperatures();
    printer.print(0, temp_sensor.getTempC(temp_addr));
    lcd.setCursor(8, 0);
    int now = ntp.getTime();
    if (now >= 0) {
      byte h = ntp.getTime()/60/60;
      byte m = ntp.getTime()/60 - h*60;
      byte s = ntp.getTime()%60;
      if (h < 10)
        lcd.print("0");
      lcd.print(h);
      lcd.print(":");
      if (m < 10)
        lcd.print("0");
      lcd.print(m);
      lcd.print(":");
      if (s < 10)
        lcd.print("0");
      lcd.print(s);
    } else {
      lcd.print("--:--:--");
    }
    updateOutDoorWeather();
  }
  ntp.handle();
  webServer.handleClient();
  ArduinoOTA.handle();
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
  content += "$('#temp').text(vars.temp + '°С\');";
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

void updateOutDoorWeather() {
  static bool overdue = true;
  static unsigned int timer = 0;
  if (overdue and http.GET() > 0) {
    String json = http.getString();
    int pos = json.indexOf("\"temp\":");
    if (pos > 0) {
      String str = json.substring(pos + 7, pos + 10);
      outdoor_temp = str.toInt();
    }
    pos = json.indexOf("\"humidity\":");
    if (pos > 0) {
      String str = json.substring(pos + 11, pos + 14);
      outdoor_humidity = str.toInt();
    }
    json = "";
    overdue = false;
    timer = 0;
  }
  if (!overdue)
    ++timer;
  if (timer >= 108000)
    overdue = true;
  if (outdoor_temp == -100 or outdoor_humidity == -100)
    return;
  lcd.setCursor(11, 1);
  if (millis()%10000 < 5000) {
    if (outdoor_temp > -10)
      lcd.print(" ");
    if (outdoor_temp >= 0 and outdoor_temp < 10)
      lcd.print(" ");
    lcd.print(outdoor_temp);
    lcd.print(char(223));
    lcd.print("C");
  } else {
    lcd.print("  ");
    if (outdoor_humidity < 10)
      lcd.print(" ");
    lcd.print(outdoor_humidity);
    lcd.print("%");
  }
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
