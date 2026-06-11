# Agents

Documenting the migration process for AI assistants working on this project.

## Context

We are migrating the project from `old_version/` (ESP8266-based workplace monitor) to a new hardware platform.

## Hardware

- **Current platform:** ESP8266
- **Display:** ST7789 (instead of the old 16x2 I2C LCD)

## Status

The root `workplace.ino` is a clean slate for the new implementation.
