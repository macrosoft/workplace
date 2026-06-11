# ESP32 Workplace Monitor

Firmware for an ESP32-based workplace information display.

## Features

- Real-time clock via NTP (UTC+3)
- Outdoor weather from Yandex Weather API (temperature, humidity, pressure, wind, sunset)
- Indoor temperature placeholder (hardcoded)
- WiFi station mode with AP fallback (`"workplace"`)
- Display: ST7789 240×320 SPI TFT via TFT_eSPI library

## Pinout

| ESP32 GPIO | Function           | TFT_eSPI define | Notes                |
|------------|--------------------|-----------------|----------------------|
| GPIO 18    | SPI clock          | `TFT_SCLK`      | default VSPI         |
| GPIO 19    | SPI MISO           | `TFT_MISO`      | default VSPI         |
| GPIO 23    | SPI MOSI           | `TFT_MOSI`      | default VSPI         |
| GPIO 15    | Chip select        | `TFT_CS`        |                      |
| GPIO 2     | Data / Command     | `TFT_DC`        |                      |
| GPIO 4     | Reset              | `TFT_RST`       |                      |
| GPIO 5     | Backlight          | `TFT_BL`        | HIGH = on, can be PWM |

> SPI pins (MOSI, MISO, SCLK) use the ESP32 default VSPI peripheral.

## Display

- **Driver:** ST7789
- **Resolution:** 240 × 320 (portrait), rotated to landscape (`setRotation(1)`)
- **SPI frequency:** 27 MHz
- **Library:** TFT_eSPI (custom `User_Setup.h` included in repo)

## Configuration

All secrets and location settings are in `config.h`:

| Define            | Description            |
|-------------------|------------------------|
| `WIFI_SSID`       | WiFi network name      |
| `WIFI_PASS`       | WiFi password          |
| `USERNAME`        | (reserved)             |
| `PASSWORD`        | AP mode password       |
| `LATITUDE`        | Latitude for weather   |
| `LONGITUDE`       | Longitude for weather  |
| `YANDEX_API_KEY`  | Yandex Weather API key |
