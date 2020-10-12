# BOSCH280
ESP8266 nonOS SDK module for BOSCH280 i²c sensors BME280 &amp; BMP280


Example project just checks if any i2c bosch devices connected to ESP8266. (Checks are based on i2c device ID)
Starts measurement on first device
Checks for measurements each 15sec and prints that in json format to serial

esp-nonos-sdk is included: folder esp-open-sdk

dockerbuild.sh: Docker build script builds code using 'vowstar/esp8266' container.
build example: ./dockerbuild.sh make
cleanup example: ./dockerbuild.sh make clean 

flash_esp01.sh & flash_esp12.sh: Scripts uses 'vowstar/esp8266' container to flash ESP8266 device (check tty device in script)
example: ./flash_esp12.sh
for ESP-01 & ESP-07 use flash_esp01.sh script

BOSCH280.creator, etc: Qt Creator IDE project file





