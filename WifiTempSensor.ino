#include <ArduinoJson.hpp>
#include <ThingsBoard.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <ESP8266HTTPUpdateServer.h>
#include <Servo.h>
#include <esp8266httpclient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
struct tcp_pcb;
extern struct tcp_pcb* tcp_tw_pcbs;
extern "C" void tcp_abort(struct tcp_pcb* pcb);

class CustomLogger {
public:
	static void log(const char *error) {
		Serial.print("[Custom Logger] ");
		Serial.println(error);
	}
};
void tcpCleanup()
{
	while (tcp_tw_pcbs != NULL)
	{
		tcp_abort(tcp_tw_pcbs);
	}
}
extern "C" {
#include "user_interface.h"
#include "WifiTempSensor.h"
}

#define CONTROL_HTML "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"><style>.form button {\r\n            margin: 15px;\r\n            height: 50px;\r\n            width: 100px;\r\n        }</style><script>function turnOn(){let n=new XMLHttpRequest;n.open(\"POST\",\"/output-on\"),n.send()}function turnOff(){let n=new XMLHttpRequest;n.open(\"POST\",\"/output-off\"),n.send()}function oneSecOn(){turnOn(),setTimeout(()=>turnOff(),1e3)}window.turnOn=turnOn,window.turnOff=turnOff,window.oneSecOn=oneSecOn;</script></head><body><div class=\"form\"><button onclick=\"turnOn()\">On</button> <button onclick=\"turnOff()\">Off</button> <button onclick=\"oneSecOn()\">On 1 sec</button></div></body></html>"

#define CONFIGURE_HTML "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"></head><body><div class=\"form\">SSID: <select id=\"ssid\"></select><br>Password: <input type=\"text\" id=\"password\" placeholder=\"Password\"><br>Alias: <input type=\"text\" id=\"alias\" placeholder=\"Alias\"><br>Server: <input type=\"text\" id=\"server\" placeholder=\"Server\"><br>MODE: <select id=\"mode\"><option value=\"0\">Servo</option><option value=\"1\">Input switch</option><option value=\"2\">Thermometer</option><option value=\"3\">Soil sensor</option><option value=\"4\">Relay</option><option value=\"5\">RGB LED</option></select><br><button type=\"button\" class=\"md-button md-button-raised\" onclick=\"submit()\">Submit</button> <a href=\"/color\">Pick colors</a></div><script>const fields=[\"ssid\",\"alias\",\"server\",\"password\",\"mode\"];function submit(){let e=new XMLHttpRequest;e.addEventListener(\"load\",function(){console.log(this.responseText)}),e.open(\"POST\",\"/configure\"),e.setRequestHeader(\"Content-type\",\"application/x-www-form-urlencoded\");let n=\"\";for(var t of fields){var o=document.getElementById(t).value;n+=`${t}=${encodeURIComponent(o)}&`}e.send(n)}function loadWifi(){var e=new XMLHttpRequest;e.addEventListener(\"load\",function(){var e=JSON.parse(this.responseText),n=e.networks;const t=document.getElementById(\"ssid\");for(var o,s=0;s<n.length;s++){var d=n[s],i=document.createElement(\"option\");i.value=d.ssid,i.innerHTML=d.ssid+\" \"+d.encryption+\" (\"+d.rssi+\")\",t.appendChild(i)}for(o of fields){var r=e[o];const a=document.getElementById(o);a.value=r}}),e.open(\"GET\",\"/currentConfig\"),e.send()}loadWifi();</script></body></html>";

const int BUTTON_PIN = 4;
const int RED_PIN = 12;
const int GREEN_PIN = 13;
const int SENSE_POWER_PIN = 14;
const int AUX_PIN = 5;

IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
boolean configMode = false;
ESP8266WebServer HTTP(80);
ESP8266HTTPUpdateServer httpUpdater;
HTTPClient httpClient;
const int ID_EEPROM_POSITION = 101;
const int HAS_SET_ID_EEPROM_POSITION = 100;
const int HAS_SET_SSID_EEPROM_POSITION = 101;
const int EEPROM_MODE_POSITION = 200;
const int EEPROM_ALIAS_POSITION = EEPROM_MODE_POSITION + 10;
const int EEPROM_SERVER_POSITION = EEPROM_ALIAS_POSITION + 255;
const int EEPROM_SSID_POSITION = EEPROM_SERVER_POSITION + 255;
const int EEPROM_PASSWORD_POSITION = EEPROM_SSID_POSITION + 255;

const int SSID_SET_VALUE = 233;

const int MODE_SERVO = 0;
const int MODE_INPUT_SWITCH = 1;
const int MODE_THERMOMETER = 2;
const int MODE_SOIL_SENSOR = 3;
const int MODE_RELAY = 4;
const int MODE_RGB_LED = 5;

int OperatingMode = 0;
int id = 0;
Servo servo;

const int oneWireBus = 5;
unsigned long int timeAtLastSend = 0;
unsigned long int timeAtLastCheck = 0;
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
WiFiClient wifiClient;
ThingsBoardSized<128, 32, CustomLogger> tb(wifiClient);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);
String serialNumber;

bool stayAwake = false;

void wifiHotspotMode() {
	digitalWrite(GREEN_PIN, HIGH);
	Serial.println("Wifi setup begin");
	WiFi.disconnect();
	disableWifiAp();
	Serial.println("Entering WIFI Config mode");

	WiFi.softAPConfig(local_IP, gateway, subnet);

	String wifiLight = "WiFiSense_";
	String ssid = wifiLight + id;
	WiFi.softAP(ssid.c_str(), "password");

	WiFi.enableAP(true);
	IPAddress myIP = WiFi.softAPIP();
	Serial.print("AP IP address: ");
	Serial.println(myIP);
	configMode = true;
	stayAwake = true;
	Serial.println("Starting http");
	HTTP.begin();
	Serial.println("HTTP Started");
}

void setMode(int mode)
{
	EEPROM.write(EEPROM_MODE_POSITION, mode);
	EEPROM.commit();
}

void checkConfigure() {
	String ssid = HTTP.arg("ssid");
	String password = HTTP.arg("password");
	String alias = HTTP.arg("alias");
	String server = HTTP.arg("server");
	byte mode = (byte)HTTP.arg("mode").toInt();

	Serial.println("Got SSID: " + ssid);
	Serial.println("Got password: " + password);
	Serial.println("Got alias: " + alias);
	Serial.println("Got server: " + server);
	Serial.print("Got mode: ");
	Serial.println(mode);

	saveWifi(ssid, password);
	setMode(mode);
	writeStringToEeprom(alias, EEPROM_ALIAS_POSITION);
	writeStringToEeprom(server, EEPROM_SERVER_POSITION);
	Serial.println("Configuration complete - rebooting...");
	HTTP.send(200, "text/plain", "OK");
	HTTP.close();
	delay(500);
	ESP.restart();
	delay(1000);
}

void registerWithServer() {
	httpClient.begin(readServerUrlFromEeprom() + "/register");
	StaticJsonDocument<200> registrationDoc;
	registrationDoc["id"] = serialNumber;
	registrationDoc["alias"] = readAliasFromEeprom();
	registrationDoc["ipAddress"] = WiFi.localIP().toString();
	registrationDoc["macAddress"] = WiFi.macAddress();
	registrationDoc["mode"] = OperatingMode;
	String registrationDocJson = "";
	serializeJson(registrationDoc, registrationDocJson);
	Serial.println("Sending: ");
	Serial.println(registrationDocJson);
	httpClient.addHeader("Content-Type", "application/json");
	int httpCode = httpClient.POST(registrationDocJson);
	if (httpCode > 0) { //Check the returning code
		Serial.println("Response: ");
		String payload = httpClient.getString();   //Get the request response payload
		Serial.println(payload);             //Print the response payload
	}

	httpClient.end();   //Close connection
}

void askServerIfShouldStayUp() {
	timeAtLastCheck = millis();
	if (configMode)
		return;
	Serial.println("Asking service if should stay up");
	String url = readServerUrlFromEeprom();

	url += "/should-remain-awake?id=";
	url += serialNumber;
	httpClient.begin(url);

	int httpCode = httpClient.GET();
	if (httpCode != 200) {
		Serial.println("Failed to get a response");
		stayAwake = false;
		return;
	}

	Serial.print("Got status code: ");
	Serial.println(httpCode);

	String payload = httpClient.getString();   //Get the request response payload
	Serial.println("Got payload: " + payload);             //Print the response payload
	if (payload == "1") {
		stayAwake = true;
	}
	if (payload == "0") {
		stayAwake = false;
	}

	httpClient.end();   //Close connection
}

void checkSetMode() {
	int mode = HTTP.arg("mode").toInt();
	Serial.print("Setting mode to ");
	Serial.println(mode);
	setMode(mode);
	HTTP.send(200, "text/plain", "OK");
	HTTP.close();
	delay(500);
	ESP.restart();
	delay(1000);
}


void handleReportRequest() {
	reportNow();
	HTTP.send(200, "text/plain", "OK");
	HTTP.close();
}

void disableWifiAp() {
	WiFi.softAPdisconnect(false);
	WiFi.enableAP(false);
}

void outputConfigureForm() {
	String homePage = CONFIGURE_HTML;
	HTTP.send(200, "text/html", homePage);
}

void outputYes() {
	HTTP.send(200, "text/html", "yes");
}

void handleOutputOn() {
	digitalWrite(SENSE_POWER_PIN, HIGH);
	HTTP.send(200, "text/html", "OK");
}

void handleOutputOff() {
	digitalWrite(SENSE_POWER_PIN, LOW);
	HTTP.send(200, "text/html", "OK");
}

void outputControl() {
	HTTP.send(200, "text/html", CONTROL_HTML);
}

void checkForIndex() {
	String homePage = "";
	if (configMode) {
		homePage = CONFIGURE_HTML;
	}
	else {
		homePage = "Serial number: " + serialNumber + "<br> Alias: " + readAliasFromEeprom();
	}
	HTTP.send(200, "text/html", homePage);
}

String readSsidFromEeprom() {
	return readStringFromEeprom(EEPROM_SSID_POSITION);
}

String readServerUrlFromEeprom() {
	return readStringFromEeprom(EEPROM_SERVER_POSITION);
}

String readAliasFromEeprom() {
	return readStringFromEeprom(EEPROM_ALIAS_POSITION);
}

String readPasswordFromEeprom() {
	return readStringFromEeprom(EEPROM_PASSWORD_POSITION);
}

void saveWifi(String ssid, String password) {
	EEPROM.write(HAS_SET_SSID_EEPROM_POSITION, SSID_SET_VALUE);
	writeStringToEeprom(ssid, EEPROM_SSID_POSITION);
	writeStringToEeprom(password, EEPROM_PASSWORD_POSITION);
}

void sendNetworksAndConfig() {
	int totalNetworks = WiFi.scanNetworks();
	StaticJsonDocument<1024> configDoc;
	JsonArray networksArray = configDoc.createNestedArray("networks");
	StaticJsonDocument<1024> thisNetwork;
	for (int i = 0; i < totalNetworks; i++) {

		thisNetwork["ssid"] = WiFi.SSID(i);
		thisNetwork["rssi"] = WiFi.RSSI(i);
		thisNetwork["encryption"] = getEncryptionName(WiFi.encryptionType(i));
		networksArray.add(thisNetwork);
	}

	configDoc["storedSsid"] = readSsidFromEeprom();
	configDoc["alias"] = readStringFromEeprom(EEPROM_ALIAS_POSITION);
	configDoc["server"] = readStringFromEeprom(EEPROM_SERVER_POSITION);
	configDoc["mode"] = readStringFromEeprom(OperatingMode);
	String json;
	serializeJson(configDoc, json);
	HTTP.send(200, "text/json", json);
}


void writeStringToEeprom(String input, int startPos) {
	Serial.print("Writing EEPROM string ");
	Serial.print(input);
	Serial.print(" at position ");
	Serial.print(startPos);
	Serial.print(" length ");
	int length = input.length();
	Serial.println(length);
	EEPROM.write(startPos, length);
	for (int i = 0; i < length; ++i)
	{
		EEPROM.write(i + 1 + startPos, input.charAt(i));
	}
	EEPROM.commit();
}

String readStringFromEeprom(int position) {
	Serial.print("Reading EEPROM string at position ");
	Serial.print(position);
	String output = "";
	int actualLength = EEPROM.read(position);
	Serial.print(" with length ");
	Serial.print(actualLength);
	Serial.print(": ");
	for (int i = position; i < position + actualLength; ++i)
	{
		char theChar = char(EEPROM.read(i + 1));
		Serial.print(theChar);
		output += theChar;
	}

	Serial.println();
	return output;
}

String getEncryptionName(byte type) {
	String typeString = "Unknown type: " + String(type);
	switch (type)
	{
	case ENC_TYPE_TKIP:
		typeString = "TKIP (WPA)";
		break;
	case ENC_TYPE_WEP:
		typeString = "WEP";
		break;
	case ENC_TYPE_CCMP:
		typeString = "CCMP (WPA)";
		break;
	case ENC_TYPE_NONE:
		typeString = "None";
		break;
	case ENC_TYPE_AUTO:
		typeString = "Auto";
		break;
	}
	return typeString;
}

void clearEeprom() {
	Serial.println("CLEARING EEPROM");
	writeStringToEeprom("", EEPROM_ALIAS_POSITION);
	writeStringToEeprom("", EEPROM_PASSWORD_POSITION);
	writeStringToEeprom("", EEPROM_SERVER_POSITION);
	writeStringToEeprom("", EEPROM_SSID_POSITION);
	writeStringToEeprom("", EEPROM_PASSWORD_POSITION);
	EEPROM.commit();
}

void printDebugInfo() {
	uint32_t realSize = ESP.getFlashChipRealSize();
	uint32_t ideSize = ESP.getFlashChipSize();
	FlashMode_t ideMode = ESP.getFlashChipMode();

	Serial.printf("CPU Freq MHz: %u", ESP.getCpuFreqMHz());

	Serial.printf("Flash real id:   %08X\n", ESP.getFlashChipId());
	Serial.printf("Flash real size: %u\n\n", realSize);

	Serial.printf("Flash ide  size: %u\n", ideSize);
	Serial.printf("Flash ide speed: %u\n", ESP.getFlashChipSpeed());
	Serial.printf("Flash ide mode:  %s\n", (ideMode == FM_QIO ? "QIO" : ideMode == FM_QOUT ? "QOUT" : ideMode == FM_DIO ? "DIO" : ideMode == FM_DOUT ? "DOUT" : "UNKNOWN"));

	if (ideSize != realSize) {
		Serial.println("Flash Chip configuration wrong!\n");
	}
	else {
		Serial.println("Flash Chip configuration ok.\n");
	}

	Serial.println();
}

void initialisePins() {
	pinMode(GREEN_PIN, OUTPUT);
	pinMode(RED_PIN, OUTPUT);
	pinMode(SENSE_POWER_PIN, OUTPUT);
	pinMode(AUX_PIN, OUTPUT);
	pinMode(BUTTON_PIN, INPUT_PULLUP);

	digitalWrite(GREEN_PIN, LOW);
	digitalWrite(RED_PIN, LOW);
	digitalWrite(SENSE_POWER_PIN, LOW);
}

void handleInitButtonPress() {
	unsigned long buttonPressedStart = millis();

	while (!digitalRead(BUTTON_PIN)) {
		delay(100);
		unsigned long buttonPressDuration = millis() - buttonPressedStart;
		if (buttonPressDuration > 1000) {
			digitalWrite(GREEN_PIN, HIGH); // Disable sleep
			stayAwake = true;
		}
		if (buttonPressDuration > 10000) {
			// Hard reset
			Serial.println("Hard reset detected");
			clearEeprom();
			digitalWrite(RED_PIN, HIGH);
			delay(1000);
			ESP.restart();
			return;
		}
	}
}
void initIdAndHostName() {
	int hasSetId = EEPROM.read(HAS_SET_ID_EEPROM_POSITION);
	Serial.print("Device configured value: ");
	Serial.println(hasSetId);
	Serial.println();
	if (hasSetId != 1) {
		Serial.println("First start configuration run. Generating ID.");
		int id = random(10000, 99999);
		EEPROM.write(ID_EEPROM_POSITION, lowByte(id));
		EEPROM.write(ID_EEPROM_POSITION + 1, highByte(id));
		EEPROM.write(HAS_SET_ID_EEPROM_POSITION, 1);
		EEPROM.commit();
	}

	id = word(EEPROM.read(ID_EEPROM_POSITION), EEPROM.read(ID_EEPROM_POSITION + 1));
	Serial.print("WIFI Sense Loaded Device ID: ");
	Serial.println(id);

	String hostname = "WiFi_Omni_";
	hostname += id;
	WiFi.hostname(hostname);
}

void initSerialNumber() {
	serialNumber = "LT1";
	serialNumber += WiFi.macAddress();
	serialNumber += "";
	serialNumber += id;
	serialNumber.replace(":", "");
	Serial.print("Serial number: ");
	Serial.println(serialNumber);
}
void initHttpServerRoutes() {
	HTTP.on("/", checkForIndex);
	HTTP.on("/is-up", outputYes);
	HTTP.on("/output-on", HTTP_POST, handleOutputOn);
	HTTP.on("/output-off", HTTP_POST, handleOutputOff);
	HTTP.on("/control", HTTP_GET, outputControl);
	HTTP.on("/wifi", HTTP_GET, outputConfigureForm);
	HTTP.on("/configure", HTTP_POST, checkConfigure);
	HTTP.on("/report", HTTP_GET, handleReportRequest);
	HTTP.on("/currentConfig", HTTP_GET, sendNetworksAndConfig);
	HTTP.on("/setMode", HTTP_POST, checkSetMode);
	HTTP.on("/description.xml", HTTP_GET, []() {
		SSDP.schema(HTTP.client());
	});
	httpUpdater.setup(&HTTP);
	HTTP.begin();
}
void setUpSsdp() {
	Serial.printf("Starting SSDP...\n");
	SSDP.setSchemaURL("description.xml");
	SSDP.setHTTPPort(80);
	String name = "WiFi Omni ";
	SSDP.setName(name + id);
	SSDP.setSerialNumber(serialNumber);
	SSDP.setURL("/");
	SSDP.setModelName("WiFi Omni V1");
	SSDP.setModelNumber("1");
	SSDP.setModelURL("http://paulmh.co.uk/wifi-omni.html");
	SSDP.setManufacturer("Paul Morris-Hill");
	SSDP.setManufacturerURL("http://paulmh.co.uk");
	SSDP.setDeviceType("upnp:rootdevice");
	SSDP.begin();
}

void connectUsingSavedWifiCreds() {
	String ssid = readSsidFromEeprom();
	String password = readPasswordFromEeprom();

	// Connect to Wi-Fi network with SSID and password
	Serial.print("Connecting to ");
	Serial.println(ssid);
	WiFi.setOutputPower(20.5);
	WiFi.setPhyMode(WIFI_PHY_MODE_11N);

	int hasSsid = EEPROM.read(HAS_SET_SSID_EEPROM_POSITION);
	if (hasSsid != SSID_SET_VALUE) {
		Serial.println("Wifi creds not stored entering wifi config mode.");
		clearEeprom();
		wifiHotspotMode();
		return;
	}

	WiFi.begin(ssid.c_str(), password.c_str());
	while (WiFi.status() != WL_CONNECTED) {
		int status = WiFi.status();
		delay(500);

		Serial.print(".");
	}

	// Print local IP address and start web server
	Serial.println("");
	Serial.println("WiFi connected.");
	Serial.println("IP address: ");
	Serial.println(WiFi.localIP());
}

void initMode() {
	OperatingMode = EEPROM.read(EEPROM_MODE_POSITION);
	Serial.print("Loaded in mode ");
	Serial.println(OperatingMode);
}

void setup() {
	Serial.begin(115200);
	EEPROM.begin(1024);
	Serial.println("\nOmnisensor\n");
	initMode();
	initialisePins();
	randomSeed(analogRead(A0));
	initIdAndHostName();
	initSerialNumber();

	// Check for button pressed
	bool buttonPressed = !digitalRead(BUTTON_PIN);
	if (buttonPressed) {
		handleInitButtonPress();
	}

	initHttpServerRoutes();
	setUpSsdp();
	connectUsingSavedWifiCreds();
	registerWithServer();
	sensors.requestTemperatures();
	sensors.getTempCByIndex(0); // First value is wrong
}

void makeSureConnectedToThingsBoard() {
	if (!tb.connected()) {
		Serial.println("Connecting to ThingsBoard node ...");

		if (tb.connect("192.168.1.116", serialNumber.c_str())) {
			Serial.println("[DONE]");
		}
	}
}

void recordTemp() {
	sensors.requestTemperatures();
	float temperatureC = sensors.getTempCByIndex(0);
	Serial.print(temperatureC);
	makeSureConnectedToThingsBoard();
	tb.sendTelemetryFloat("temperature", temperatureC);
}

void recordSoil() {
	delay(100); // Delay prevents voltage drop on 3.3v line when using capacitive soil sensor
	digitalWrite(SENSE_POWER_PIN, HIGH);
	delay(100);
	int soil = analogRead(A0);
	digitalWrite(SENSE_POWER_PIN, LOW);
	Serial.print("Read soil: ");
	Serial.println(soil);
	makeSureConnectedToThingsBoard();
	tb.sendTelemetryInt("analogVoltage", soil);
}

void reportNow() {
	Serial.println("Reporting");
	if (OperatingMode == MODE_THERMOMETER) {
		Serial.println("Recording temp");
		recordTemp();
	}

	Serial.println("Recording analog");
	recordSoil();

	tb.loop();
	timeAtLastSend = millis();
}

void loop() {
	if (stayAwake) {
		HTTP.handleClient();

		if (millis() - timeAtLastSend > 30 * 1000) {
			reportNow();
		}
		if (millis() - timeAtLastCheck > 30 * 1000) {
			askServerIfShouldStayUp();
		}
	}

	if (!stayAwake) {
		askServerIfShouldStayUp();
	}

	if (!stayAwake) {
		reportNow();
		Serial.print("Been up for ");
		Serial.print(millis());
		Serial.println(" milliseconds");
		Serial.println("Sleeping...");
		ESP.deepSleep(1 * 60 * 1000000);
		return;
	}

	unsigned long int timeRunning = millis();
	if (timeRunning > 86400 * 1000) {
		Serial.println("Restarting due to running too long");
		ESP.restart();
		return;
	}
}


