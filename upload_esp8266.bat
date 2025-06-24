@echo off
echo Uploading firmware to ESP8266 on COM5...
platformio run -e esp12f --target upload --upload-port COM5
echo Upload complete.
pause