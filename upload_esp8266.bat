@echo off
echo Uploading firmware to ESP8266 on COM6...
platformio run -e esp12f --target upload --upload-port COM6
echo Upload complete.
pause