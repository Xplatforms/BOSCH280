#!/bin/bash

docker run --rm -v `pwd`:/build -w /build -e SDK_BASE=esp-open-sdk/sdk vowstar/esp8266 $*
