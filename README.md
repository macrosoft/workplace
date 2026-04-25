# Workplace

ESP8266-based workplace monitor with LCD display, LED indicator, and weather integration.

## Features

- **Temperature sensor** – Dallas Temperature (OneWire) with big digits display on LCD
- **Real-time clock** – NTP synchronization with Russian and Windows time servers
- **Outdoor weather** – Yandex Weather API integration (temperature and humidity)
- **RGB LED** – Chainable LED with auto-brightness based on ambient light sensor
- **Web dashboard** – Built-in web server with AJAX updates
- **OTA updates** – Arduino OTA and HTTP firmware update server
- **AP fallback** – Access point mode if WiFi connection fails

## Hardware

- ESP8266 (NodeMCU/Wemos D1)
- 16x2 LCD display with I2C backlight (0x3F)
- Chainable LED matrix
- Dallas Temperature sensor (DS18B20)
- Photoresistor on A0

## Pinout

| Pin | Function |
|-----|----------|
| D0  | LED CLK  |
| D1  | LED DATA |
| D2  | I2C SDA  |
| D3  | I2C SCL  |
| D4  | OneWire  |
| A0  | Light sensor |

## Setup

1. Copy `config.h.example` to `config.h`
2. Fill in WiFi credentials, passwords, coordinates, and Yandex API key
3. Upload firmware via Arduino IDE
4. Upload SPIFFS data: tools → ESP8266 Sketch Data Upload

## Web Interface

After connection, access the device at its IP address to view light level and temperature readings.

## OTA Update

```bash
# Via Arduino IDE
# Tools → Port → select workplace

# Via HTTP
curl -F "image=@firmware.bin" http://admin:password@<ip>/update
```

## Libraries

- ChainableLED
- ESP8266WiFi / ESP8266HTTPClient
- FS (SPIFFS)
- OneWire + DallasTemperature
- ArduinoOTA
- LiquidCrystal_I2C
