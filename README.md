# Digital Tantrum

A cardboard robot toy box on an ESP32 that reacts emotionally to temperature.
Built at UBISS 2026 (Robot Makers workshop).

## What it does
The box "throws a tantrum" based on what it senses. A caregiver scans an RFID
card to unlock it; tilt the box into tantrum mode and it reads temperature,
shows a mood face on an 8x8 LED matrix (smile / sad + HOT/COLD/AHA words),
and buzzes when overheating. The temperature is also streamed wirelessly over
ESP-NOW to a second ESP32 that displays it on a 4-digit 7-segment readout.

## Hardware
- 2x ESP32-WROVER (Freenove)
- MFRC522 RFID reader
- 10k NTC thermistor (Steinhart-Hart conversion)
- 8x8 LED matrix (74HC595 shift registers)
- 4-digit 7-segment display (multiplexed, direct drive)
- Tilt switch, passive buzzer

## Architecture
- **sender/** — Tantrum box: RFID auth, thermistor, matrix faces, buzzer.
  Broadcasts temperature via ESP-NOW.
- **receiver/** — Display board: receives temperature over ESP-NOW,
  shows it on the 7-segment display.

## Setup
1. Flash `receiver/receiver.ino` first, note its MAC from Serial.
2. Put that MAC in `receiverMac[]` in `sender/sender.ino`, flash the Tantrum board.
3. Both boards use Arduino-ESP32 core v3.x.

## Notes
- Segments avoid GPIO12/0/2/15 where possible (boot-strapping pins).
