# Agents

Guidance for AI assistants working on this project.

## Context

The migration from `old_version/` (ESP8266 + 16x2 I2C LCD) is **complete**. The firmware was
fully rewritten for ESP32 + ST7789 SPI TFT and `old_version/` has been removed from the repo.
The root `workplace.ino` is the active, feature-complete implementation.

## Hardware

- **Platform:** ESP32 (classic, WROOM-32 style DevKit)
- **Display:** ST7789 240x320 SPI TFT, driven by TFT_eSPI, `setRotation(3)` -> 320x240 landscape
- **Sensors/actuators:** DS18B20 indoor temp, photoresistor on ADC, P9813 RGB LED strip
- Full pin table: see "Pinout" in README.md (also duplicated as defines at the top of `workplace.ino`)

## Files

| File           | Purpose                                                              |
|----------------|----------------------------------------------------------------------|
| `workplace.ino`| All firmware logic (setup/loop, UI, sensors, HTTP, LED)              |
| `config.h`     | Secrets & location (WiFi, AP password, lat/lon, Yandex key, TV IP)   |
| `ntp.h`        | `NtpClient` — RU NTP pools, re-sync every 6 h, drift-free via millis() offset |
| `webpage.h`    | `INDEX_HTML` PROGMEM config page served from flash                   |
| `User_Setup.h` | TFT_eSPI user setup; must be copied into the TFT_eSPI library folder to build |

## Subsystems

- **Clock:** `NtpClient` bakes UTC+3 into `getTime()` (hardcoded `+10800` in `parseNtpPacket()`);
  `localtime()` is only formatting. `updateClock()` redraws only changed strings.
- **Weather:** Yandex Weather informer over HTTPS every 30 min; JSON parsed manually with
  `indexOf()` per key in `updateOutDoorWeather()`. Several fields (feels_like, pressure, wind,
  sunset) are fetched but not displayed yet — intended for future dashboard tiles.
- **UI rendering:** draw into sprites (`dashSprite` 308x90), then `pushSprite()`; value-based
  dirty flags inside a 1 s UI tick. Icons are vector primitives (`drawThermoIcon`,
  `drawDropIcon`, `drawHouseIcon`). A color frame around the screen mirrors the LED strip color.
- **LED (P9813):** color slowly cycles hue (`updateLedColor()` + per-boot random offset);
  brightness is smoothed in a 20 ms fade tick in `loop()`. Target: 0 (light off) / 0.05 (TV on) / 1.0.
- **Light sensor:** photoresistor with configurable hysteresis thresholds
  (`cfg.lightLow/high`, persisted in SPIFFS, tuned from the web page).
- **TV presence:** TCP-probes `TV_IP:3001` every 5 s while the desk light is on;
  3 consecutive failures = offline (debounce).
- **Web server (port 80):** `GET /` config page, `POST /save` (thresholds -> SPIFFS, hue offset
  in RAM), `GET /ajax` JSON telemetry. WiFi failure at boot falls back to AP `"workplace"`.
- **OTA:** ArduinoOTA, hostname `workplace` (password protected in AP mode only).

## Timing summary

| Task                | Period        |
|---------------------|---------------|
| LED fade tick       | 20 ms         |
| UI tick             | 1 s           |
| DS18B20 read        | 10 s          |
| TV probe            | 5 s (while light on) |
| Weather fetch       | 30 min        |
| NTP re-sync         | 6 h           |

## Gotchas

- `config.h` is tracked in git and contains **real credentials** — do not copy its values into
  logs, issues, or new files.
- TFT_eSPI setup: the repo's `User_Setup.h` replaces the one inside the TFT_eSPI library
  (or enable it via `User_Setup_Select.h`). Without this, the display stays black/garbled.
- Manual JSON parsing depends on Yandex response key names; re-check `updateOutDoorWeather()`
  when touching weather fields.
- Keep `loop()` non-blocking: add work as a dirty-flagged periodic tick, render via sprites,
  avoid `delay()` outside boot.
- SPIFFS stores only the binary `/config` blob (the `Config` struct); no SPIFFS data upload is
  required to flash. Changing `Config` layout breaks existing stored configs (acceptable,
  defaults are sane).
- Text metrics: default font cell is `6 * textSize` px wide — budget dashboard rows accordingly
  (this is why the indoor temp `C` suffix was tight on width).

## Status / possible next steps

- Display already-fetched weather fields (feels-like, pressure, wind, sunset) as tiles.
- Weather condition icons from the Yandex `icon` field (day/night + clear/cloudy/rain).
- Smooth (.vlw) fonts from SPIFFS for the clock; light-level gauge bar; indoor-temp sparkline.
- Sync README with code: rotation is `setRotation(3)` in `workplace.ino` (README says 1).
