#include "SensorManager.h"

SensorManager::SensorManager(int oneWirePin, int powerPin) : 
    oneWireBus(oneWirePin), 
    sensePowerPin(powerPin) {
    oneWire = new OneWire(oneWireBus);
    sensors = new DallasTemperature(oneWire);
}

SensorManager::~SensorManager() {
    delete sensors;
    delete oneWire;
}

void SensorManager::init() {
    pinMode(sensePowerPin, OUTPUT);
    digitalWrite(sensePowerPin, LOW);
    
    // Configure A0 for analog reading
    #ifdef ESP32_PLATFORM
        pinMode(A0, INPUT);
    #endif
    
    sensors->begin();
    // First reading is often inaccurate, so do a dummy read
    sensors->requestTemperatures();
    sensors->getTempCByIndex(0);
}

float SensorManager::readTemperature() {
    sensors->requestTemperatures();
    float temperatureC = sensors->getTempCByIndex(0);
    Serial.print("Temperature: ");
    Serial.print(temperatureC);
    Serial.println("°C");
    return temperatureC;
}

int SensorManager::readSoilMoisture() {
    // Delay prevents voltage drop on 3.3v line when using capacitive soil sensor
    delay(100);
    powerSensorOn();
    delay(100);
    int soil = analogRead(A0);
    powerSensorOff();
    Serial.print("Read soil: ");
    Serial.println(soil);
    return soil;
}

void SensorManager::powerSensorOn() {
    digitalWrite(sensePowerPin, HIGH);
}

void SensorManager::powerSensorOff() {
    digitalWrite(sensePowerPin, LOW);
}