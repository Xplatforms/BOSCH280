#!/bin/bash
docker run --rm -v `pwd`:/build -w /build --device /dev/ttyUSB0:/dev/ttyUSB0 -e SDK_BASE=esp-open-sdk/sdk vowstar/esp8266 esptool.py --port /dev/ttyUSB0 --baud 115200 write_flash --flash_freq 40m --flash_mode dio --flash_size 32m 0x00000 firmware/0x00000.bin 0x10000 firmware/0x10000.bin
