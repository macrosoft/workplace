#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoOTA.h>

#include "config.h"
#include "ntp.h"

#define DS18B20_PIN 16
#define PHOTO_PIN 34
#define LED_CLK_PIN 13
#define LED_DATA_PIN 14

struct Config {
  char ssid[32] = {0};
  char pass[64] = {0};
  uint16_t lightLow = 1200;
  uint16_t lightHigh = 1600;
};

Config cfg;

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

TFT_eSPI tft = TFT_eSPI();

NtpClient ntp;
WiFiClientSecure secureClient;
WiFiServer server(80);

unsigned long lastScreenUpdate = 0;
unsigned long lastWeatherUpdate = 0;
bool firstWeatherDone = false;

bool isFirstWeatherDraw = true;

int outdoor_temp = -100;
int outdoor_humidity = -100;
int outdoor_feels_like = -100;
int outdoor_pressure = -100;
float outdoor_wind_speed = -100.0;
String outdoor_sunset = "--:--";

float indoor_temp = 0.0;
unsigned long lastSensorRead = 0;

int lightLevel = 0;
bool lightState = false;

float offsetHue;
float v = 1.0;
float saturation = 1.0;
uint8_t ledR, ledG, ledB;

void setup() {
  Serial.begin(115200);
  Serial.println("\n[SYSTEM] Booting ESP32...");

  sensors.begin();

  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);

  pinMode(LED_CLK_PIN, OUTPUT);
  pinMode(LED_DATA_PIN, OUTPUT);

  if (!SPIFFS.begin(true)) {
    Serial.println("[SPIFFS] Mount failed, using defaults");
  } else {
    fs::File f = SPIFFS.open("/config", "r");
    if (f) {
      f.read((uint8_t*)&cfg, sizeof(cfg));
      f.close();
      Serial.println("[SPIFFS] Config loaded");
    } else {
      Serial.println("[SPIFFS] No config file, using defaults");
    }
  }

  offsetHue = (float)esp_random() / UINT32_MAX;

  tft.init();
  tft.invertDisplay(false);
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);

  drawInterfaceSkeleton();

  updateClock();
  updateWiFiStatus();
  drawWeatherDashboard();

  const char* ssid = cfg.ssid[0] ? cfg.ssid : WIFI_SSID;
  const char* pass = cfg.ssid[0] ? cfg.pass : WIFI_PASS;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  Serial.print("[WIFI] Connecting to ");
  Serial.print(ssid);

  bool connected = false;
  for (byte i = 0; i < 40; i++) {
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(250);
    Serial.print(".");
  }

  if (connected) {
    Serial.println(" [OK]");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
    ntp.begin();
    Serial.println("[NTP] Started");
  } else {
    Serial.println(" [FAILED]");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("workplace", PASSWORD);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  secureClient.setInsecure();
  updateWiFiStatus();

  server.begin();
  Serial.println("[HTTP] Server started");

  ArduinoOTA.setHostname("workplace");
  if (!connected) ArduinoOTA.setPassword(PASSWORD);
  ArduinoOTA.begin();
}

void loop() {
  ntp.handle();
  handleWebClient();
  ArduinoOTA.handle();

  if (millis() - lastSensorRead >= 10000) {
    lastSensorRead = millis();
    sensors.requestTemperatures();
    indoor_temp = sensors.getTempCByIndex(0);
  }

  if (WiFi.status() == WL_CONNECTED) {
    if (!firstWeatherDone || (millis() - lastWeatherUpdate >= 1800000UL)) {
      lastWeatherUpdate = millis();
      firstWeatherDone = true;
      updateOutDoorWeather();
    }
  }

  lightLevel = analogRead(PHOTO_PIN);
  if (!lightState && lightLevel > cfg.lightHigh)
    lightState = true;
  else if (lightState && lightLevel < cfg.lightLow)
    lightState = false;

  if (millis() - lastScreenUpdate >= 1000) {
    lastScreenUpdate = millis();

    updateClock();
    updateWiFiStatus();
    updateLedColor();
    if (lightState)
      ledWrite(ledR, ledG, ledB);
    else
      ledWrite(0, 0, 0);
    drawWeatherDashboard();
  }

  delay(1);
}

void drawInterfaceSkeleton() {
  tft.drawRect(0, 0, 320, 240, TFT_DARKGREY);
}

void updateWiFiStatus() {
  tft.setTextSize(1);     
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
    tft.fillRect(2, 2, 316, 15, TFT_BLACK);
  } else if (WiFi.getMode() == WIFI_AP) {
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.print("AP MODE: Connect to 'workplace'        ");
  } else {
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.print("WiFi: Offline - Reconnecting...        ");
  }
}

void updateClock() {
  unsigned long epochTime = ntp.getTime();

  if (epochTime == 0) {
    tft.setTextSize(5); 
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(40, 25); 
    tft.print("--:--:--");
    return;
  }

  time_t rawtime = (time_t)epochTime;
  struct tm * ti;
  ti = localtime(&rawtime);

  char timeString[9];
  sprintf(timeString, "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);
  
  tft.setTextSize(5); 
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(40, 25); 
  tft.print(timeString);

  char dateString[32];
  const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  sprintf(dateString, "%s, %02d %s %04d   ", 
          days[ti->tm_wday], 
          ti->tm_mday, 
          months[ti->tm_mon], 
          ti->tm_year + 1900);

  tft.setTextSize(2); 
  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK); 
  
  int strWidth = strlen(dateString) * 12;
  int xPos = (320 - strWidth) / 2 + 10; 
  
  tft.setCursor(xPos, 70); 
  tft.print(dateString);
}

void drawDegreeSymbol(int x, int y, int radius, uint16_t color) {
  tft.drawCircle(x, y + radius, radius, color);
  tft.drawCircle(x, y + radius, radius - 1, color);
}

void updateLedColor() {
  float hue = fmod(millis() / 21600000.0 + offsetHue, 1.0);
  byte i = hue * 6;
  float f = hue * 6 - i;
  float p = v * (1 - saturation);
  float q = v * (1 - f * saturation);
  float t = v * (1 - (1 - f) * saturation);
  switch (i % 6) {
    case 0: ledR = v*255; ledG = t*255; ledB = p*255; break;
    case 1: ledR = q*255; ledG = v*255; ledB = p*255; break;
    case 2: ledR = p*255; ledG = v*255; ledB = t*255; break;
    case 3: ledR = p*255; ledG = q*255; ledB = v*255; break;
    case 4: ledR = t*255; ledG = p*255; ledB = v*255; break;
    case 5: ledR = v*255; ledG = p*255; ledB = q*255; break;
  }
}

void ledWrite(uint8_t r, uint8_t g, uint8_t b) {
  digitalWrite(LED_DATA_PIN, LOW);
  for (byte i = 0; i < 32; i++) {
    digitalWrite(LED_CLK_PIN, LOW);
    digitalWrite(LED_CLK_PIN, HIGH);
  }

  uint32_t data = ((uint32_t)0xC3 << 24) | ((uint32_t)b << 16) | ((uint32_t)g << 8) | r;
  for (byte i = 0; i < 32; i++) {
    digitalWrite(LED_DATA_PIN, (data & 0x80000000) ? HIGH : LOW);
    digitalWrite(LED_CLK_PIN, LOW);
    digitalWrite(LED_CLK_PIN, HIGH);
    data <<= 1;
  }

  digitalWrite(LED_DATA_PIN, LOW);
  for (byte i = 0; i < 32; i++) {
    digitalWrite(LED_CLK_PIN, LOW);
    digitalWrite(LED_CLK_PIN, HIGH);
  }
}

void drawWeatherDashboard() {
  if (outdoor_temp == -100 || outdoor_humidity == -100) {
    tft.setTextSize(2);
    tft.setCursor(100, 150);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("Loading...");
    return;
  }

  if (isFirstWeatherDraw) {
    tft.fillRect(2, 100, 316, 135, TFT_BLACK);
    isFirstWeatherDraw = false;
  }

  tft.setTextSize(3);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  
  char outTempStr[8];
  sprintf(outTempStr, "%+d", outdoor_temp); 
  
  tft.fillRect(70, 130, 50, 25, TFT_BLACK);
  
  tft.setCursor(20, 130); 
  tft.print(outTempStr);
  
  int outCx = tft.getCursorX(); 
  drawDegreeSymbol(outCx + 5, 130, 3, TFT_YELLOW);
  
  tft.setCursor(outCx + 14, 130);
  tft.print("C");

  tft.setTextSize(2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  char outHumStr[12];
  sprintf(outHumStr, "%d%%  ", outdoor_humidity);
  tft.setCursor(20, 175);
  tft.print(outHumStr);


  tft.setTextSize(4);
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  
  char inTempStr[8];
  if (indoor_temp < -50 || indoor_temp > 150) {
    sprintf(inTempStr, "--.-");
  } else {
    sprintf(inTempStr, "%.1f", indoor_temp);
  }
  
  tft.fillRect(240, 125, 70, 35, TFT_BLACK);

  tft.setCursor(160, 125); 
  tft.print(inTempStr);

  if (indoor_temp >= -50 && indoor_temp <= 150) {
    int inCx = tft.getCursorX();
    drawDegreeSymbol(inCx + 6, 125, 4, TFT_ORANGE);
    tft.setCursor(inCx + 18, 125);
    tft.print("C");
  }

  tft.fillRect(230, 165, 86, 50, TFT_BLACK);
  int cx = 255, cy = 190, r = 10;
  if (lightState) {
    tft.fillCircle(cx, cy, r, tft.color565(ledR, ledG, ledB));
  } else {
    tft.drawCircle(cx, cy, r, TFT_DARKGREY);
    tft.drawCircle(cx, cy, r - 1, TFT_DARKGREY);
    tft.drawCircle(cx, cy, r - 2, TFT_DARKGREY);
  }
}

void updateOutDoorWeather() {
  if (WiFi.status() != WL_CONNECTED) return;  
  Serial.println("[HTTP] Requesting Yandex weather...");
  HTTPClient https;
  String url = "https://api.weather.yandex.ru/v1/informers?lat=" + String(LATITUDE) + "&lon=" + String(LONGITUDE);
  if (https.begin(secureClient, url)) {
    https.addHeader("X-Yandex-API-Key", YANDEX_API_KEY);
    int httpCode = https.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      String json = https.getString();
      
      int tempPos = json.indexOf("\"temp\":");
      if (tempPos > 0) {
        int start = tempPos + 7;
        int end = json.indexOf(",", start);
        if (end > start) outdoor_temp = json.substring(start, end).toInt();
      }
      
      int feelsPos = json.indexOf("\"feels_like\":");
      if (feelsPos > 0) {
        int start = feelsPos + 13;
        int end = json.indexOf(",", start);
        if (end > start) outdoor_feels_like = json.substring(start, end).toInt();
      }
      
      int humPos = json.indexOf("\"humidity\":");
      if (humPos > 0) {
        int start = humPos + 11;
        int end = json.indexOf(",", start);
        if (end > start) outdoor_humidity = json.substring(start, end).toInt();
      }

      int pressPos = json.indexOf("\"pressure_mm\":");
      if (pressPos > 0) {
        int start = pressPos + 14;
        int end = json.indexOf(",", start);
        if (end > start) outdoor_pressure = json.substring(start, end).toInt();
      }

      int windPos = json.indexOf("\"wind_speed\":");
      if (windPos > 0) {
        int start = windPos + 13;
        int end = json.indexOf(",", start);
        if (end > start) outdoor_wind_speed = json.substring(start, end).toFloat();
      }

      int sunsetPos = json.indexOf("\"sunset\":\"");
      if (sunsetPos > 0) {
        int start = sunsetPos + 10;
        int end = json.indexOf("\"", start);
        if (end > start) outdoor_sunset = json.substring(start, end);
      }

      Serial.printf("[HTTP] Sync OK. Temp: %d C\n", outdoor_temp);
    } else {
      Serial.printf("HTTP error: %d (%s)\n",
                  httpCode,
                  https.errorToString(httpCode).c_str());
    }
    https.end();
  }
}

void handleWebClient() {
  WiFiClient client = server.available();
  if (!client) return;

  String method = "";
  String path = "";
  String body = "";
  int contentLength = 0;

  while (client.connected() && !client.available()) delay(1);

  if (client.available()) {
    String reqLine = client.readStringUntil('\n');
    reqLine.trim();
    int s1 = reqLine.indexOf(' ');
    int s2 = reqLine.indexOf(' ', s1 + 1);
    if (s1 > 0 && s2 > s1) {
      method = reqLine.substring(0, s1);
      path = reqLine.substring(s1 + 1, s2);
    }

    while (true) {
      String line = client.readStringUntil('\n');
      if (line == "\r" || line == "") break;
      if (line.startsWith("Content-Length:") || line.startsWith("content-length:")) {
        contentLength = line.substring(line.indexOf(':') + 1).toInt();
      }
    }

    if (contentLength > 0) {
      body = client.readString();
      body.trim();
    }
  }

  if (method == "GET" && path == "/") {
    String html = "<!DOCTYPE html><html><head><meta charset=\"utf-8\">";
    html += "<title>Workplace</title>";
    html += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    html += "<style>";
    html += "body{font-family:Arial;margin:20px;background:#222;color:#ddd}";
    html += "h1{color:#0f0}table{width:100%;border-collapse:collapse}";
    html += "td{padding:8px 4px}.lbl{color:#999;font-size:.9em}";
    html += ".val{font-size:1.5em;font-weight:bold;text-align:right}";
    html += ".sec{margin:20px 0;padding:15px;background:#333;border-radius:8px}";
    html += "input{width:100%;padding:8px;margin:4px 0 12px;box-sizing:border-box}";
    html += "input[type=submit]{background:#070;color:#fff;border:0;padding:10px;font-size:1em;cursor:pointer}";
    html += "label{font-weight:bold}";
    html += ".swatch{width:60px;height:60px;border-radius:50%;border:2px solid #555;flex-shrink:0}";
    html += ".row{display:flex;align-items:center;gap:15px}";
    html += "input[type=range]{width:100%}";
    html += "</style></head><body>";
    html += "<h1>Workplace</h1>";
    html += "<div class=\"sec\"><table>";
    html += "<tr><td class=\"lbl\">Indoor</td><td class=\"val\" id=\"in\">--.-</td><td>&deg;C</td></tr>";
    html += "<tr><td class=\"lbl\">Outdoor</td><td class=\"val\" id=\"out\">--</td><td>&deg;C</td></tr>";
    html += "<tr><td class=\"lbl\">Humidity</td><td class=\"val\" id=\"hum\">--</td><td>%</td></tr>";
    html += "<tr><td class=\"lbl\">Light</td><td class=\"val\" id=\"light\">--</td><td></td></tr>";
    html += "</table></div>";
    html += "<div class=\"sec\">";
    html += "<h2>Light thresholds</h2>";
    html += "<label>ON (dark) <input type=\"number\" id=\"lo\" value=\"" + String(cfg.lightLow) + "\"></label>";
    html += "<label>OFF (bright) <input type=\"number\" id=\"hi\" value=\"" + String(cfg.lightHigh) + "\"></label>";
    html += "<input type=\"submit\" value=\"Apply\" onclick=\"save()\">";
    html += "<p id=\"msg\"></p>";
    html += "</div>";
    html += "<div class=\"sec\">";
    html += "<h2>LED Color</h2>";
    html += "<div class=\"row\">";
    html += "<div class=\"swatch\" id=\"swatch\"></div>";
    html += "<div style=\"flex:1\">";
    html += "<label>Hue offset <span id=\"hueVal\">0</span>&deg;</label>";
    html += "<input type=\"range\" id=\"oh\" min=\"0\" max=\"360\" value=\"0\" oninput=\"setHue(this.value)\">";
    html += "</div></div></div>";
    html += "<script>";
    html += "function save(){";
    html += "var b='lo='+document.getElementById('lo').value+'&hi='+document.getElementById('hi').value;";
    html += "fetch('/save',{method:'POST',body:b,headers:{'Content-Type':'application/x-www-form-urlencoded'}})";
    html += ".then(function(r){document.getElementById('msg').textContent='Saved'});}";
    html += "function setHue(v){";
    html += "document.getElementById('hueVal').textContent=v;";
    html += "fetch('/save',{method:'POST',body:'oh='+(v/360),headers:{'Content-Type':'application/x-www-form-urlencoded'}});}";
    html += "setInterval(function(){";
    html += "fetch('/ajax').then(function(r){return r.json()}).then(function(d){";
    html += "document.getElementById('in').textContent=d.i;";
    html += "document.getElementById('out').textContent=d.o;";
    html += "document.getElementById('hum').textContent=d.h;";
    html += "document.getElementById('light').textContent=d.l;";
    html += "document.getElementById('swatch').style.background='rgb('+d.r+','+d.g+','+d.b+')';";
    html += "document.getElementById('oh').value=Math.round(d.oh*360);";
    html += "document.getElementById('hueVal').textContent=Math.round(d.oh*360);";
    html += "})},2000);";
    html += "</script></body></html>";
    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n");
    client.print(html);

  } else if (method == "POST" && path == "/save") {
    int p1, p2;

    p1 = body.indexOf("lo=");
    if (p1 >= 0) {
      p1 += 3;
      p2 = body.indexOf("&", p1);
      if (p2 < 0) p2 = body.length();
      cfg.lightLow = body.substring(p1, p2).toInt();
    }

    p1 = body.indexOf("hi=");
    if (p1 >= 0) {
      p1 += 3;
      p2 = body.indexOf("&", p1);
      if (p2 < 0) p2 = body.length();
      cfg.lightHigh = body.substring(p1, p2).toInt();
    }

    p1 = body.indexOf("oh=");
    if (p1 >= 0) {
      p1 += 3;
      p2 = body.indexOf("&", p1);
      if (p2 < 0) p2 = body.length();
      offsetHue = body.substring(p1, p2).toFloat();
    }

    fs::File f = SPIFFS.open("/config", "w");
    if (f) {
      f.write((uint8_t*)&cfg, sizeof(cfg));
      f.close();
    }

    client.print("HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nConnection: close\r\n\r\nOK");

  } else if (method == "GET" && path == "/ajax") {
    String json = "{";
    json += "\"i\":\"" + String(indoor_temp, 1) + "\"";
    json += ",\"o\":\"" + String(outdoor_temp) + "\"";
    json += ",\"h\":\"" + String(outdoor_humidity) + "\"";
    json += ",\"l\":\"" + String(lightLevel) + "\"";
    json += ",\"r\":\"" + String(ledR) + "\"";
    json += ",\"g\":\"" + String(ledG) + "\"";
    json += ",\"b\":\"" + String(ledB) + "\"";
    json += ",\"oh\":\"" + String(offsetHue, 4) + "\"";
    json += "}";
    client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nConnection: close\r\n\r\n");
    client.print(json);

  } else {
    client.print("HTTP/1.1 404 Not Found\r\nConnection: close\r\n\r\n");
  }

  client.stop();
}