#include <WiFi.h>              // Было ESP8266WiFi.h
#include <HTTPClient.h>        // Было ESP8266HTTPClient.h
#include <WiFiClientSecure.h> 
#include <SPI.h>
#include <TFT_eSPI.h>          // ЗАМЕНА Adafruit на TFT_eSPI!
#include <OneWire.h>
#include <DallasTemperature.h>

#include "config.h"
#include "ntp.h"

#define DS18B20_PIN 16
#define PHOTO_PIN 34

OneWire oneWire(DS18B20_PIN);
DallasTemperature sensors(&oneWire);

// TFT_eSPI инициализируется без указания пинов здесь, 
// они берутся из User_Setup.h!
TFT_eSPI tft = TFT_eSPI(); 

NtpClient ntp;
WiFiClientSecure secureClient; 

unsigned long lastScreenUpdate = 0;
unsigned long lastWeatherUpdate = 0;
bool firstWeatherDone = false;

// Глобальная переменная для отслеживания первой отрисовки
bool isFirstWeatherDraw = true; 

// Переменные погоды Яндекса
int outdoor_temp = -100;
int outdoor_humidity = -100;
int outdoor_feels_like = -100;
int outdoor_pressure = -100;
float outdoor_wind_speed = -100.0;
String outdoor_sunset = "--:--";

float indoor_temp = 0.0;
unsigned long lastSensorRead = 0;

int lightLevel = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("\n[SYSTEM] Booting ESP32...");

  sensors.begin();

  // Включение подсветки дисплея (GPIO 5)
  // В будущем сюда можно подать ШИМ (ledc) для регулировки яркости
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH); 

  // Инициализация дисплея ILI9341
  tft.init();
  tft.invertDisplay(false);
  tft.setRotation(1); // Пейзажная ориентация
  tft.fillScreen(TFT_BLACK);

  // Рисуем статичный интерфейс
  drawInterfaceSkeleton();

  // Начальная прорисовка (плейсхолдеры)
  updateClock();
  updateWiFiStatus();
  drawWeatherDashboard();

  // Подключение к Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WIFI] Connecting to ");
  Serial.print(WIFI_SSID);

  bool connected = false;
  for (byte i = 0; i < 40; i++) { // 10 seconds timeout
    if (WiFi.status() == WL_CONNECTED) {
      connected = true;
      break;
    }
    delay(250);
    Serial.print(".");
  }

  if (connected) {
    Serial.println(" [OK]");
    ntp.begin(); 
    Serial.println("[NTP] Custom NTP client started.");
  } else {
    Serial.println(" [FAILED]");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("workplace", PASSWORD);
    Serial.println("[WIFI] Access Point 'workplace' started.");
  }

  secureClient.setInsecure(); // Пропуск проверки SSL сертификата (нужно для Яндекса)
  updateWiFiStatus();
}

void loop() {
  ntp.handle();

  // Чтение DS18B20 раз в 10 секунд
  if (millis() - lastSensorRead >= 10000) {
    lastSensorRead = millis();
    sensors.requestTemperatures();
    indoor_temp = sensors.getTempCByIndex(0);
  }

  // Запрос погоды раз в 30 минут (только если есть Wi-Fi)
  if (WiFi.status() == WL_CONNECTED) {
    if (!firstWeatherDone || (millis() - lastWeatherUpdate >= 1800000UL)) {
      lastWeatherUpdate = millis();
      firstWeatherDone = true;
      updateOutDoorWeather();
    }
  }

  // Обновление экрана раз в секунду
  if (millis() - lastScreenUpdate >= 1000) {
    lastScreenUpdate = millis();
    
    lightLevel = analogRead(PHOTO_PIN);

    updateClock();
    updateWiFiStatus();
    drawWeatherDashboard();
    
    // Вывод в консоль
    Serial.print("[SYSTEM] WiFi: ");
    Serial.print(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED");
    Serial.print(" | Time: ");
    
    int totalSeconds = ntp.getTime();
    if (totalSeconds >= 0) {
      int hours = (totalSeconds / 3600) % 24;
      int minutes = (totalSeconds / 60) % 60;
      int seconds = totalSeconds % 60;
      Serial.printf("%02d:%02d:%02d", hours, minutes, seconds);
    } else {
      Serial.print("NOT SYNCED");
    }

    if (outdoor_temp != -100) {
      Serial.printf(" | Weather: %+dC, %d%%, %dmmHg, %.1fm/s, Sunset: %s\n", 
                    outdoor_temp, outdoor_humidity, outdoor_pressure, outdoor_wind_speed, outdoor_sunset.c_str());
    } else {
      Serial.println(" | Weather: NO DATA");
    }
  }
  
  delay(1); 
}

void drawInterfaceSkeleton() {
  // Только одна рамка по самому краю экрана
  tft.drawRect(0, 0, 320, 240, TFT_DARKGREY);
}

void updateWiFiStatus() {
  tft.setTextSize(1);     
  if (WiFi.getMode() == WIFI_STA && WiFi.status() == WL_CONNECTED) {
    // Безопасно стираем строку, не задевая верхнюю рамку (y=2)
    tft.fillRect(2, 2, 316, 15, TFT_BLACK); 
  } else if (WiFi.getMode() == WIFI_AP) {
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_RED, TFT_BLACK);
    // Добавлены пробелы в конце для затирки артефактов!
    tft.print("AP MODE: Connect to 'workplace'        ");
  } else {
    tft.setCursor(10, 10);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    // Добавлены пробелы в конце для затирки артефактов!
    tft.print("WiFi: Offline - Reconnecting...        ");
  }
}

void updateClock() {
  // Получаем полный Unix Time из нашего кастомного ntp.h
  unsigned long epochTime = ntp.getTime();

  if (epochTime == 0) {
    tft.setTextSize(5); 
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.setCursor(40, 25); 
    tft.print("--:--:--");
    return;
  }

  // Конвертируем Unix Time в структуру времени C++
  time_t rawtime = (time_t)epochTime;
  struct tm * ti;
  ti = localtime(&rawtime);

  // 1. Рисуем ЧАСЫ
  char timeString[9];
  sprintf(timeString, "%02d:%02d:%02d", ti->tm_hour, ti->tm_min, ti->tm_sec);
  
  tft.setTextSize(5); 
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setCursor(40, 25); 
  tft.print(timeString);

  // 2. Рисуем ДАТУ и ДЕНЬ НЕДЕЛИ
  char dateString[32];
  const char* days[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
  const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

  // Формат: "Monday, 25 Oct 2023"
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

void drawWeatherDashboard() {
  if (outdoor_temp == -100 || outdoor_humidity == -100) {
    tft.setTextSize(2);
    tft.setCursor(100, 150);
    tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
    tft.print("Loading...");
    return;
  }

  if (isFirstWeatherDraw) {
    // Безопасная очистка всей нижней половины (не задевая рамку)
    tft.fillRect(2, 100, 316, 135, TFT_BLACK); 
    isFirstWeatherDraw = false;
  }

  // ==========================================
  // ЛЕВАЯ ЧАСТЬ (Улица)
  // ==========================================
  
  tft.setTextSize(3); // Размер уменьшен
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  
  char outTempStr[8];
  sprintf(outTempStr, "%+d", outdoor_temp); 
  
  // Локальная зачистка старого градуса
  tft.fillRect(70, 130, 50, 25, TFT_BLACK); 
  
  tft.setCursor(20, 130); 
  tft.print(outTempStr);
  
  int outCx = tft.getCursorX(); 
  // Радиус градуса уменьшен до 3
  drawDegreeSymbol(outCx + 5, 130, 3, TFT_YELLOW);
  
  tft.setCursor(outCx + 14, 130);
  tft.print("C");

  // Влажность
  tft.setTextSize(2); // Размер уменьшен
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  char outHumStr[12];
  // Два пробела в конце работают как ластик для артефактов
  sprintf(outHumStr, "%d%%  ", outdoor_humidity);
  tft.setCursor(20, 175);
  tft.print(outHumStr);


  // ==========================================
  // ПРАВАЯ ЧАСТЬ (Дом)
  // ==========================================
  
  tft.setTextSize(4); // Размер уменьшен с 5 до 4
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  
  char inTempStr[8];
  if (indoor_temp < -50 || indoor_temp > 150) {
    sprintf(inTempStr, "--.-");
  } else {
    sprintf(inTempStr, "%.1f", indoor_temp);
  }
  
  // Локальная зачистка старого градуса Дома
  tft.fillRect(240, 125, 70, 35, TFT_BLACK);

  tft.setCursor(160, 125); 
  tft.print(inTempStr);

  if (indoor_temp >= -50 && indoor_temp <= 150) {
    int inCx = tft.getCursorX();
    drawDegreeSymbol(inCx + 6, 125, 4, TFT_ORANGE);
    tft.setCursor(inCx + 18, 125);
    tft.print("C");
  }

  // Фоторезистор
  tft.setTextSize(2);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.fillRect(230, 175, 86, 20, TFT_BLACK);
  tft.setCursor(230, 175);
  tft.print(lightLevel);
}

// Парсинг Яндекса остается без изменений, он отлично работает на ESP32
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
      Serial.printf("[HTTP] GET failed, error: %d\n", httpCode);
    }
    https.end();
  }
}