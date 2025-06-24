#include "EEPROMManager.h"

EEPROMManager::EEPROMManager() {
}

void EEPROMManager::init() {
    EEPROM.begin(1024);
}

bool EEPROMManager::hasDeviceId() {
    return EEPROM.read(HAS_SET_ID_EEPROM_POSITION) == 1;
}

int EEPROMManager::getDeviceId() {
    return word(EEPROM.read(ID_EEPROM_POSITION), EEPROM.read(ID_EEPROM_POSITION + 1));
}

void EEPROMManager::setDeviceId(int id) {
    EEPROM.write(ID_EEPROM_POSITION, lowByte(id));
    EEPROM.write(ID_EEPROM_POSITION + 1, highByte(id));
    EEPROM.write(HAS_SET_ID_EEPROM_POSITION, 1);
    EEPROM.commit();
}

bool EEPROMManager::hasWiFiCredentials() {
    return EEPROM.read(HAS_SET_SSID_EEPROM_POSITION) == SSID_SET_VALUE;
}

String EEPROMManager::getSSID() {
    return readString(EEPROM_SSID_POSITION);
}

String EEPROMManager::getPassword() {
    return readString(EEPROM_PASSWORD_POSITION);
}

void EEPROMManager::saveWiFiCredentials(String ssid, String password) {
    EEPROM.write(HAS_SET_SSID_EEPROM_POSITION, SSID_SET_VALUE);
    writeString(ssid, EEPROM_SSID_POSITION);
    writeString(password, EEPROM_PASSWORD_POSITION);
}

int EEPROMManager::getMode() {
    int mode = EEPROM.read(EEPROM_MODE_POSITION);
    // If EEPROM is uninitialized (255), return default mode
    if (mode == 255) {
        return 2; // Default to MODE_THERMOMETER
    }
    return mode;
}

void EEPROMManager::setMode(int mode) {
    EEPROM.write(EEPROM_MODE_POSITION, mode);
    EEPROM.commit();
}

String EEPROMManager::getAlias() {
    return readString(EEPROM_ALIAS_POSITION);
}

void EEPROMManager::setAlias(String alias) {
    writeString(alias, EEPROM_ALIAS_POSITION);
}

String EEPROMManager::getServerUrl() {
    return readString(EEPROM_SERVER_POSITION);
}

void EEPROMManager::setServerUrl(String server) {
    writeString(server, EEPROM_SERVER_POSITION);
}

bool EEPROMManager::hasServerUrl() {
    return hasStringAt(EEPROM_SERVER_POSITION);
}

void EEPROMManager::addWiFiFailure(unsigned long timestamp) {
    String currentLog = getWiFiFailureLog();
    
    // If log is empty, start with empty array
    if (currentLog.length() == 0) {
        currentLog = "[]";
    }
    
    // Parse existing log and add new timestamp
    String newLog = currentLog;
    if (newLog == "[]") {
        newLog = "[" + String(timestamp) + "]";
    } else {
        // Remove the closing bracket and add new timestamp
        newLog = newLog.substring(0, newLog.length() - 1);
        newLog += "," + String(timestamp) + "]";
    }
    
    Serial.println("Adding WiFi failure timestamp: " + String(timestamp));
    Serial.println("Updated failure log: " + newLog);
    
    writeString(newLog, EEPROM_FAILURE_LOG_POSITION);
}

String EEPROMManager::getWiFiFailureLog() {
    return readString(EEPROM_FAILURE_LOG_POSITION);
}

void EEPROMManager::clearWiFiFailureLog() {
    Serial.println("Clearing WiFi failure log");
    writeString("", EEPROM_FAILURE_LOG_POSITION);
}

void EEPROMManager::clearAll() {
    Serial.println("CLEARING EEPROM");
    writeString("", EEPROM_ALIAS_POSITION);
    writeString("", EEPROM_PASSWORD_POSITION);
    writeString("", EEPROM_SERVER_POSITION);
    writeString("", EEPROM_SSID_POSITION);
    writeString("", EEPROM_PASSWORD_POSITION);
    writeString("", EEPROM_FAILURE_LOG_POSITION);
    
    // Clear the WiFi credentials flag so hasWiFiCredentials() returns false
    EEPROM.write(HAS_SET_SSID_EEPROM_POSITION, 0);
    
    EEPROM.commit();
}

void EEPROMManager::writeString(String input, int startPos) {
    Serial.print("Writing EEPROM string ");
    Serial.print(input);
    Serial.print(" at position ");
    Serial.print(startPos);
    Serial.print(" length ");
    int length = input.length();
    Serial.println(length);
    EEPROM.write(startPos, length);
    for (int i = 0; i < length; ++i) {
        EEPROM.write(i + 1 + startPos, input.charAt(i));
    }
    EEPROM.commit();
}

String EEPROMManager::readString(int position) {
    Serial.print("Reading EEPROM string at position ");
    Serial.print(position);
    String output = "";
    int actualLength = EEPROM.read(position);
    
    // Check if length is valid (not uninitialized EEPROM)
    if (actualLength == 255 || actualLength < 0) {
        Serial.println(" with invalid length (uninitialized): \"\"");
        return "";
    }
    
    Serial.print(" with length ");
    Serial.print(actualLength);
    Serial.print(": \"");
    for (int i = 0; i < actualLength; ++i) {
        char theChar = char(EEPROM.read(position + 1 + i));
        Serial.print(theChar);
        output += theChar;
    }
    Serial.println("\"");
    return output;
}

bool EEPROMManager::hasStringAt(int position) {
    int length = EEPROM.read(position);
    return length != 255 && length > 0;
}