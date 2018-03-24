/*********
	Rui Santos
	Complete project details at http://randomnerdtutorials.com
*********/


#include <ESP8266WiFi.h>
#include "FastLED.h"
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266SSDP.h>
#include <ESP8266HTTPUpdateServer.h>

#define DATA_PIN 5
// Replace with your network credentials
struct tcp_pcb;
extern struct tcp_pcb* tcp_tw_pcbs;
extern "C" void tcp_abort(struct tcp_pcb* pcb);

void tcpCleanup()
{
		while (tcp_tw_pcbs != NULL)
		{
				tcp_abort(tcp_tw_pcbs);
		}
}
extern "C" {
		#include "user_interface.h"
}

#define COLOR_PICKER_HTML "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"><style>@import url(https://fonts.googleapis.com/css?family=Open+Sans);body{margin:0;padding:0;height:100%}#color-block{position:absolute;left:0;top:0}#color-strip{position:absolute;top:0;left:70%}canvas:hover{cursor:crosshair}</style></head><body><div id=\"color-picker\"><canvas id=\"color-block\"></canvas><canvas id=\"color-strip\"></canvas></div><script>var isSettingColor=!1;function rgbToHex(t,e,o){return((1<<24)+(t<<16)+(e<<8)+o).toString(16).slice(1)}function setColor(t){if(!isSettingColor){isSettingColor=!0;var e=new XMLHttpRequest;e.onreadystatechange=function(){4===e.readyState?(isSettingColor=!1,200===e.status&&console.log(e.responseText)):console.log(\"Error: \"+e.status)},e.open(\"POST\",\"/set-all\"),e.setRequestHeader(\"Content-type\",\"application/x-www-form-urlencoded\"),e.send(\"color=\"+t)}}window.addEventListener(\"DOMContentLoaded\",function(){console.log(\"HERE\");var t=document.getElementById(\"color-block\"),e=t.getContext(\"2d\");t.width=.7*window.innerWidth,t.height=window.innerHeight;var o=t.width,n=t.height,r=document.getElementById(\"color-strip\"),a=r.getContext(\"2d\");r.width=.3*window.innerWidth,r.height=window.innerHeight;var d=r.width,i=r.height,l=0,s=0,c=!1,g=\"rgba(255,0,0,1)\";e.rect(0,0,o,n),f(),a.rect(0,0,d,i);var u=a.createLinearGradient(0,0,0,n);function f(){e.fillStyle=g,e.fillRect(0,0,o,n);var t=a.createLinearGradient(0,0,o,0);t.addColorStop(0,\"rgba(255,255,255,1)\"),t.addColorStop(1,\"rgba(255,255,255,0)\"),e.fillStyle=t,e.fillRect(0,0,o,n);var r=a.createLinearGradient(0,0,0,n);r.addColorStop(0,\"rgba(0,0,0,0)\"),r.addColorStop(1,\"rgba(0,0,0,1)\"),e.fillStyle=r,e.fillRect(0,0,o,n)}function h(t){console.log(\"Mouse down\",t),c=!0,v(t)}function C(t){c&&v(t)}function S(t){c=!1}function v(t){l=t.offsetX,s=t.offsetY,t.touches&&(l=t.touches[0].clientX,s=t.touches[0].clientY,t.preventDefault());var o=e.getImageData(l,s,1,1).data;g=\"rgba(\"+o[0]+\",\"+o[1]+\",\"+o[2]+\",1)\",setColor(rgbToHex(o[0],o[1],o[2])),console.log(\"Color chosen\",g)}u.addColorStop(0,\"rgba(255, 0, 0, 1)\"),u.addColorStop(.17,\"rgba(255, 255, 0, 1)\"),u.addColorStop(.34,\"rgba(0, 255, 0, 1)\"),u.addColorStop(.51,\"rgba(0, 255, 255, 1)\"),u.addColorStop(.68,\"rgba(0, 0, 255, 1)\"),u.addColorStop(.85,\"rgba(255, 0, 255, 1)\"),u.addColorStop(1,\"rgba(255, 0, 0, 1)\"),a.fillStyle=u,a.fill(),r.addEventListener(\"click\",function(t){l=t.offsetX,s=t.offsetY;var e=a.getImageData(l,s,1,1).data;g=\"rgba(\"+e[0]+\",\"+e[1]+\",\"+e[2]+\",1)\",f()},!1),t.addEventListener(\"mousedown\",h,!1),t.addEventListener(\"touchstart\",h,!1),t.addEventListener(\"mouseup\",S,!1),t.addEventListener(\"touchend\",S,!1),t.addEventListener(\"mousemove\",C,!1),t.addEventListener(\"touchmove\",C,!1)});</script></body></html>";
#define CONFIGURE_HTML "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width,initial-scale=1,maximum-scale=1,user-scalable=no\"></head><body><div class=\"form\">SSID: <select id=\"ssid\"></select><br>Password: <input type=\"text\" id=\"password\" placeholder=\"Password\"><br><button type=\"button\" class=\"md-button md-button-raised\" onclick=\"submit()\">Submit</button> <a href=\"/color\">Pick colors</a></div><script>var ssidSelector=document.getElementById(\"ssid\"),networks=[];function getSelectedSsid(){return networks[ssidSelector.selectedIndex].ssid}function submit(){var e=document.getElementById(\"password\").value;var n=new XMLHttpRequest;n.addEventListener(\"load\",function(){console.log(this.responseText)});var t=encodeURIComponent(getSelectedSsid()),s=encodeURIComponent(e);n.open(\"POST\",\"/configure\"),n.setRequestHeader(\"Content-type\",\"application/x-www-form-urlencoded\"),n.send(\"ssid=\"+t+\"&password=\"+s)}function loadWifi(){var e=new XMLHttpRequest;e.addEventListener(\"load\",function(){networks=JSON.parse(this.responseText);for(var e=0;e<networks.length;e++){var n=networks[e],t=document.createElement(\"option\");t.value=n.ssid,t.innerHTML=n.ssid+\" \"+n.encryption+\" (\"+n.rssi+\")\",ssidSelector.appendChild(t)}}),e.open(\"GET\",\"/networks\"),e.send()}loadWifi();</script></body></html>";

// Auxiliar variables to store the current output state
const int RESET_PIN = 4;
// Assign output variables to GPIO pins
#define NUM_LEDS 20
#define ANIMATION_NONE 0
#define ANIMATION_FILL_UP 1
#define ANIMATION_MIDDLE_OUT 2
#define ANIMATION_OUTWARD_IN 3

CRGB leds[NUM_LEDS];
CRGB Orange = CRGB(12, 6, 0);
CRGB Blue = CRGB(0, 0, 18);
CRGB Red = CRGB(18, 0, 0);
CRGB Green = CRGB(0, 18, 0);
IPAddress local_IP(192, 168, 1, 1);
IPAddress gateway(192, 168, 1, 1);
IPAddress subnet(255, 255, 255, 0);
boolean configMode = false;
ESP8266WebServer HTTP(80);
ESP8266HTTPUpdateServer httpUpdater;
int AnimationMode = ANIMATION_NONE;
int animationDelay = 12;
String flashingColor1;
String flashingColor2;
const int ID_EEPROM_POSITION = 101;
const int HAS_SET_ID_EEPROM_POSITION = 100;
int id = 0;

boolean isFlashing = false;
boolean currentFlashingState = false;
unsigned long int lastFlashChange = 0;
int flashingDelay;

void writeLeds() {
		noInterrupts();
		FastLED.show();
		interrupts();
}

void wifiSetup() {
		WiFi.disconnect();
		disableWifiAp();
		leds[0] = Orange; // orange
		writeLeds();
		Serial.println("Entering WIFI Config mode");

		WiFi.softAPConfig(local_IP, gateway, subnet);

		String wifiLight = "WiFiLight_";
		String ssid = wifiLight + id;
		WiFi.softAP(ssid.c_str(), "password");

		WiFi.enableAP(true);
		IPAddress myIP = WiFi.softAPIP();
		Serial.print("AP IP address: ");
		Serial.println(myIP);

		leds[0] = Blue;
		writeLeds();
		configMode = true;
		HTTP.begin();
}


void setLedStateFromHex(int index, String hex) {
		int number = (int)strtol(&hex[0], NULL, 16);

		// Split them up into r, g, b values
		int r = number >> 16;
		int g = number >> 8 & 0xFF;
		int b = number & 0xFF;
		leds[index] = CRGB(r, g, b);
		/*Serial.print("R: ");
		Serial.print(r);
		Serial.print(" G: ");
		Serial.print(g);
		Serial.print(" B: ");
		Serial.println(b);*/
}

void checkConfigure() {
		String ssid = HTTP.arg("ssid");
		String password = HTTP.arg("password");
		
		Serial.println("Got SSID: " + ssid);
		Serial.println("Got password: " + password);

		saveWifi(ssid, password);

		HTTP.send(200, "text/plain", "OK");
		leds[0] = Orange;
		writeLeds();
		HTTP.close();
		delay(500);
		ESP.restart();
		delay(1000);
}

void setAnimationMode() {
		if (HTTP.hasArg("mode")) {
				String animationMode = HTTP.arg("mode");
				AnimationMode = animationMode.toInt();
		}

		if (HTTP.hasArg("delay")) {
				String animationRate = HTTP.arg("delay");
				animationDelay = animationRate.toInt();
				if (animationDelay > 100) 
						animationDelay = 100;
		}
		
		HTTP.send(200, "text/plain", "OK");
}

void flash() {
		if (!HTTP.hasArg("delay") || !HTTP.hasArg("color1") || !HTTP.hasArg("color2")) {
				HTTP.send(400, "text/plain", "Needs input delay, color1, color2");
				return;
		}

		lastFlashChange = millis();
		flashingDelay = HTTP.arg("delay").toInt();
		flashingColor1 = HTTP.arg("color1");
		flashingColor2 = HTTP.arg("color2");
		isFlashing = true;
		HTTP.send(200, "text/plain", "OK");
}

void stopFlashing() {
		isFlashing = false;
		currentFlashingState = false;
		clearLeds();
}

void checkFlash() {
		unsigned long int now = millis();
		if (!isFlashing) return;

		if (!digitalRead(RESET_PIN)) {
				isFlashing = false;
				clearLeds();
				return;
		}

		if (now - lastFlashChange < flashingDelay) return;
		setAllLeds(currentFlashingState ? flashingColor1 : flashingColor2);
		lastFlashChange = lastFlashChange + flashingDelay;
		currentFlashingState = !currentFlashingState;
}

void disableWifiAp() {
		WiFi.softAPdisconnect(false);
		WiFi.enableAP(false);
}

void outputConfigureForm() {
		String homePage = CONFIGURE_HTML;
		HTTP.send(200, "text/html", homePage);
}

void checkForIndex() {
		String homePage = "";
		if (configMode) {
				homePage = CONFIGURE_HTML;
		}
		else {
				homePage = COLOR_PICKER_HTML;
		}
		HTTP.send(200, "text/html", homePage);
}

void checkColorPicker() {
		String homePage = COLOR_PICKER_HTML;
		HTTP.send(200, "text/html", homePage);
}

void checkSetAll() {
		int pixelColorStart = 0;
		if (!HTTP.hasArg("color")) {
				HTTP.send(400, "text/plain", "Bad request missing color param.");
				return;
		}
		String hex = HTTP.arg("color");

		if (hex.length() != 6) {
				HTTP.send(400, "text/plain", "BAD REQUEST: Color should be 6 digit hex.");
				return;
		}

		Serial.print("Set all to ");
		Serial.println(hex);
		setAllLeds(hex);
		writeLeds();
		HTTP.send(200, "text/plain", "Set all LEDs to " + hex);
}

void setAllLeds(String hex) {
		if (AnimationMode == ANIMATION_NONE) {
				for (int i = 0; i < NUM_LEDS; i++) {
						setLedStateFromHex(i, hex);
				}
				writeLeds();
		}

		if (AnimationMode == ANIMATION_FILL_UP) {
				for (int i = 0; i < NUM_LEDS; i++) {
						setLedStateFromHex(i, hex);
						delay(animationDelay);
						writeLeds();
				}
		}

		if (AnimationMode == ANIMATION_MIDDLE_OUT) {
				int halfLeds = NUM_LEDS / 2;
				for (int i = 0; i < halfLeds; i++) {
						setLedStateFromHex(halfLeds + i, hex);
						setLedStateFromHex(halfLeds - i - 1, hex);
						delay(animationDelay);
						writeLeds();
				}
		}

		if (AnimationMode == ANIMATION_OUTWARD_IN) {
				int halfLeds = NUM_LEDS / 2;
				for (int i = halfLeds-1; i >= 0; i--) {
						setLedStateFromHex(halfLeds + i, hex);
						setLedStateFromHex(halfLeds - i - 1, hex);
						delay(animationDelay);
						writeLeds();
				}
		}
}

void checkSet() {
		if (!HTTP.hasArg("colors")) {
				HTTP.send(400, "text/plain", "BAD REQUEST: Missing colors arg");
				return;
		}
		String colors = HTTP.arg("colors");
		int colorsLength = colors.length();
		int pixelColorStart = 0;
		int pixelIndex = 0;
		while (true) {
				if (pixelIndex >= NUM_LEDS) break;
				int pixelColorEnd = pixelColorStart + 6;
				if (pixelColorEnd > colorsLength) break;
				String hexCode = colors.substring(pixelColorStart, pixelColorEnd);

				Serial.print("Got ");
				Serial.println(hexCode);
				setLedStateFromHex(pixelIndex, hexCode);

				pixelColorStart = pixelColorEnd;

				pixelIndex++;
		}

		writeLeds();
		HTTP.send(200, "text/plain", "OK");
}

String readSsidFromEeprom() {
		Serial.println("Reading EEPROM ssid");
		String esid;
		for (int i = 0; i < 32; ++i)
		{
				esid += char(EEPROM.read(i));
		}
		Serial.print("SSID: ");
		Serial.println(esid);
		return esid;
}

String readPasswordFromEeprom() {
		Serial.println("Reading EEPROM pass");
		String epass = "";
		for (int i = 32; i < 96; ++i)
		{
				epass += char(EEPROM.read(i));
		}
		Serial.print("PASS: ");
		Serial.println(epass);
		return epass;
}

void saveWifi(String ssid, String password) {
		clearEeprom();
		for (int i = 0; i < ssid.length(); ++i)
		{
				EEPROM.write(i, ssid[i]);
				Serial.print("Wrote: ");
				Serial.println(ssid[i]);
		}
		Serial.println("writing eeprom pass:");
		for (int i = 0; i < password.length(); ++i)
		{
				EEPROM.write(32 + i, password[i]);
				Serial.print("Wrote: ");
				Serial.println(password[i]);
		}
		EEPROM.commit();
}

void sendNetworks() {
		int totalNetworks = WiFi.scanNetworks();
		String networkJson = "[";

		for (int i = 0; i < totalNetworks; i++) {
				if (i > 0) networkJson += ",";
				networkJson += "{";
				networkJson += "		\"ssid\": \"" + WiFi.SSID(i) + "\",";
				networkJson += "		\"rssi\": " + String(WiFi.RSSI(i)) + ",";
				networkJson += "		\"encryption\": \"" + getEncryptionName(WiFi.encryptionType(i)) + "\"";
				networkJson += "}";
		}

		networkJson += "]";
		HTTP.send(200, "text/json", networkJson);
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
		for (int i = 0; i < 96; ++i) { EEPROM.write(i, 0); }
}

void clearLeds() {
		for (int i = 0; i < NUM_LEDS; i++) {
				leds[i] = CRGB::Black;
		}
		writeLeds();
}

void setup() {
		EEPROM.begin(512);

		int hasSetId = EEPROM.read(HAS_SET_ID_EEPROM_POSITION);
		Serial.begin(115200);

		uint32_t realSize = ESP.getFlashChipRealSize();
		uint32_t ideSize = ESP.getFlashChipSize();
		FlashMode_t ideMode = ESP.getFlashChipMode();

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
		Serial.print("Device configured value: ");
		Serial.println(hasSetId);
		Serial.println();
		if (hasSetId == 255) {
				Serial.println("First start configuration run. Generating ID.");
				int id = random(10000, 99999);
				EEPROM.write(ID_EEPROM_POSITION, lowByte(id));
				EEPROM.write(ID_EEPROM_POSITION + 1, highByte(id));
				EEPROM.write(HAS_SET_ID_EEPROM_POSITION, 1);

				EEPROM.commit();
		}

		id = word(EEPROM.read(ID_EEPROM_POSITION), EEPROM.read(ID_EEPROM_POSITION + 1));
		Serial.print("WIFI Light Loaded Device ID: ");
		Serial.println(id);
		
		String hostname = "WiFi_Light_";
		hostname += id;
		WiFi.hostname(hostname);
		randomSeed(analogRead(A0));
		pinMode(RESET_PIN, INPUT_PULLUP);

		FastLED.addLeds<WS2811, DATA_PIN, GRB>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

		HTTP.on("/", checkForIndex);
		HTTP.on("/color", checkColorPicker);
		HTTP.on("/wifi", HTTP_GET, outputConfigureForm);
		HTTP.on("/configure", HTTP_POST, checkConfigure);
		HTTP.on("/set", HTTP_POST, checkSet);
		HTTP.on("/set-all", HTTP_POST, checkSetAll);
		HTTP.on("/flash", HTTP_POST, flash);
		HTTP.on("/stop-flashing", HTTP_POST, stopFlashing);
		HTTP.on("/set-animation-mode", HTTP_POST, setAnimationMode);
		HTTP.on("/networks", HTTP_GET, sendNetworks);
		HTTP.on("/description.xml", HTTP_GET, []() {
				SSDP.schema(HTTP.client());
		});
		httpUpdater.setup(&HTTP);
		HTTP.begin();

		Serial.printf("Starting SSDP...\n");
		SSDP.setSchemaURL("description.xml");
		SSDP.setHTTPPort(80);
		String name = "WiFi Light ";
		SSDP.setName(name + id);
		String serialNumber = "LV1_";
		serialNumber += WiFi.macAddress();
		serialNumber += "_";
		serialNumber += id;

		SSDP.setSerialNumber(serialNumber);
		SSDP.setURL("/");
		SSDP.setModelName("WiFi Light V1");
		SSDP.setModelNumber("1");
		SSDP.setModelURL("http://paulmh.co.uk/wifi-light.html");
		SSDP.setManufacturer("Paul Morris-Hill");
		SSDP.setManufacturerURL("http://paulmh.co.uk");
		SSDP.setDeviceType("upnp:rootdevice");
		SSDP.begin();

		if (!digitalRead(RESET_PIN)) {
				clearEeprom();
				wifiSetup();
				return;
		}
		disableWifiAp();

		leds[0] = Orange;
		writeLeds();

		String ssid = readSsidFromEeprom();
		String password = readPasswordFromEeprom();

		// Connect to Wi-Fi network with SSID and password
		Serial.print("Connecting to ");
		Serial.println(ssid);
		WiFi.setOutputPower(20.5);

		if (password.length() == 0) {
				Serial.println("Wifi creds not stored entering wifi config mode.");
				wifiSetup();
				return;
		}

		WiFi.begin(ssid.c_str(), password.c_str());
		while (WiFi.status() != WL_CONNECTED) {
				int status = WiFi.status();
				clearLeds();
				if (status < 20) {
						leds[status] = Red;
						writeLeds();
				}
				delay(500);
				
				Serial.print(".");
		}
		clearLeds();
		leds[0] = Green;
		writeLeds();
		delay(1000);

		leds[0] = CRGB::Black;
		writeLeds();

		// Print local IP address and start web server
		Serial.println("");
		Serial.println("WiFi connected.");
		Serial.println("IP address: ");
		Serial.println(WiFi.localIP());
}

void loop() {
		HTTP.handleClient();
		checkFlash();
}


