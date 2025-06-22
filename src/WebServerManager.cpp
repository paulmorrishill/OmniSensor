#include "WebServerManager.h"
#include "EEPROMManager.h"
#include "WiFiManager.h"
#include "SensorManager.h"
#include "DeviceManager.h"
#include "html_constants.h"

WebServerManager::WebServerManager(EEPROMManager* eeprom, WiFiManager* wifi, SensorManager* sensor, DeviceManager* device) :
    eepromManager(eeprom), wifiManager(wifi), sensorManager(sensor), deviceManager(device) {
    server = new WebServerType(80);
    httpUpdater = new HTTPUpdateServerType();
    
#ifdef ESP32_PLATFORM
    ssdpDevice = new uDevice();
    ssdpServer = new uSSDP();
#endif
}

WebServerManager::~WebServerManager() {
#ifdef ESP32_PLATFORM
    delete ssdpServer;
    delete ssdpDevice;
#endif
    delete httpUpdater;
    delete server;
}

void WebServerManager::init() {
    // Setup routes
    server->on("/", [this]() { handleIndex(); });
    server->on("/is-up", [this]() { handleIsUp(); });
    server->on("/output-on", HTTP_POST, [this]() { handleOutputOn(); });
    server->on("/output-off", HTTP_POST, [this]() { handleOutputOff(); });
    server->on("/control", HTTP_GET, [this]() { handleControl(); });
    server->on("/wifi", HTTP_GET, [this]() { handleWifi(); });
    server->on("/configure", HTTP_POST, [this]() { handleConfigure(); });
    server->on("/report", HTTP_GET, [this]() { handleReport(); });
    server->on("/currentConfig", HTTP_GET, [this]() { handleCurrentConfig(); });
    server->on("/setMode", HTTP_POST, [this]() { handleSetMode(); });
    server->on("/description.xml", HTTP_GET, [this]() { handleSSDPSchema(); });
    
    httpUpdater->setup(server);
    server->begin();
    Serial.println("HTTP Server started");
}

void WebServerManager::handleClient() {
    server->handleClient();
#ifdef ESP32_PLATFORM
    // Process SSDP for ESP32
    if (ssdpServer) {
        ssdpServer->process();
    }
#endif
}

void WebServerManager::setupSSDP(String serialNumber, int deviceId) {
    Serial.printf("Starting SSDP...\n");
    
#ifdef ESP8266_PLATFORM
    SSDP.setSchemaURL("description.xml");
    SSDP.setHTTPPort(80);
    String name = "WiFi Omni ";
    SSDP.setName(name + deviceId);
    SSDP.setSerialNumber(serialNumber);
    SSDP.setURL("/");
    SSDP.setModelName("WiFi Omni V1");
    SSDP.setModelNumber("1");
    SSDP.setModelURL("http://paulmh.co.uk/wifi-omni.html");
    SSDP.setManufacturer("Paul Morris-Hill");
    SSDP.setManufacturerURL("http://paulmh.co.uk");
    SSDP.setDeviceType("upnp:rootdevice");
    SSDP.begin();
#elif defined(ESP32_PLATFORM)
    // ESP32 SSDP setup using uSSDP-ESP32 library
    byte mac[6];
    char base[64];
    WiFi.macAddress(mac);
    sprintf(base, "esp32-%02x%02x-%02x%02x-%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    ssdpDevice->begin((const char*)base, mac);
    
    // Create mutable copies for the uSSDP library
    char serialNumberBuf[64];
    char manufacturerBuf[32] = "Paul Morris-Hill";
    char manufacturerURLBuf[64] = "http://paulmh.co.uk";
    char modelNameBuf[32] = "WiFi Omni V1";
    char presentationURLBuf[8] = "/";
    
    serialNumber.toCharArray(serialNumberBuf, sizeof(serialNumberBuf));
    String name = "WiFi Omni " + String(deviceId);
    char friendlyNameBuf[64];
    name.toCharArray(friendlyNameBuf, sizeof(friendlyNameBuf));
    
    ssdpDevice->serialNumber(serialNumberBuf);
    ssdpDevice->manufacturer(manufacturerBuf);
    ssdpDevice->manufacturerURL(manufacturerURLBuf);
    ssdpDevice->friendlyName(friendlyNameBuf);
    ssdpDevice->modelName(modelNameBuf);
    ssdpDevice->modelNumber(1, 0);
    ssdpDevice->presentationURL(presentationURLBuf);
    
    ssdpServer->begin(ssdpDevice);
    Serial.println("SSDP started for ESP32");
#endif
}

void WebServerManager::handleIndex() {
    String homePage = "";
    if (wifiManager->isInConfigMode()) {
        homePage = CONFIGURE_HTML;
    } else {
        homePage = "Serial number: " + deviceManager->getSerialNumber() + "<br> Alias: " + eepromManager->getAlias();
    }
    server->send(200, "text/html", homePage);
}

void WebServerManager::handleIsUp() {
    server->send(200, "text/html", "yes");
}

void WebServerManager::handleOutputOn() {
    if (deviceManager->getOperatingMode() == 6) { // MODE_LATCHING_VALVE
        deviceManager->openValve();
    } else {
        sensorManager->powerSensorOn();
    }
    server->send(200, "text/html", "OK");
}

void WebServerManager::handleOutputOff() {
    if (deviceManager->getOperatingMode() == 6) { // MODE_LATCHING_VALVE
        deviceManager->closeValve();
    } else {
        sensorManager->powerSensorOff();
    }
    server->send(200, "text/html", "OK");
}

void WebServerManager::handleControl() {
    server->send(200, "text/html", CONTROL_HTML);
}

void WebServerManager::handleWifi() {
    server->send(200, "text/html", CONFIGURE_HTML);
}

void WebServerManager::handleConfigure() {
    String ssid = server->arg("ssid");
    String password = server->arg("password");
    String alias = server->arg("alias");
    String serverUrl = server->arg("server");
    byte mode = (byte)server->arg("mode").toInt();

    Serial.println("Got SSID: " + ssid);
    Serial.println("Got password: " + password);
    Serial.println("Got alias: " + alias);
    Serial.println("Got server: " + serverUrl);
    Serial.print("Got mode: ");
    Serial.println(mode);

    eepromManager->saveWiFiCredentials(ssid, password);
    eepromManager->setMode(mode);
    eepromManager->setAlias(alias);
    eepromManager->setServerUrl(serverUrl);
    
    Serial.println("Configuration complete - rebooting...");
    server->send(200, "text/plain", "OK");
    server->close();
    delay(500);
    PlatformUtils::restart();
}

void WebServerManager::handleReport() {
    deviceManager->reportNow();
    server->send(200, "text/plain", "OK");
}

void WebServerManager::handleCurrentConfig() {
    StaticJsonDocument<1024> configDoc;
    JsonArray networksArray = configDoc.createNestedArray("networks");
    
    wifiManager->scanNetworks(networksArray);
    
    configDoc["storedSsid"] = eepromManager->getSSID();
    configDoc["alias"] = eepromManager->getAlias();
    configDoc["server"] = eepromManager->getServerUrl();
    configDoc["mode"] = eepromManager->getMode();
    
    String json;
    serializeJson(configDoc, json);
    server->send(200, "text/json", json);
}

void WebServerManager::handleSetMode() {
    int mode = server->arg("mode").toInt();
    Serial.print("Setting mode to ");
    Serial.println(mode);
    eepromManager->setMode(mode);
    server->send(200, "text/plain", "OK");
    server->close();
    delay(500);
    PlatformUtils::restart();
}

void WebServerManager::handleSSDPSchema() {
#ifdef ESP8266_PLATFORM
    SSDP.schema(server->client());
#elif defined(ESP32_PLATFORM)
    // ESP32 SSDP schema handling using uSSDP library
    ssdpServer->schema(server->client());
#endif
}