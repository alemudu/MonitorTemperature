#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <WiFiClient.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>
#include <Chrono.h>

const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "ESP8266_SETUP";
boolean settingMode;
String ssidList;
const int minuteInterval = 5;
Chrono chronoConnection(Chrono::SECONDS);
Chrono chronoSendData(Chrono::SECONDS);

DNSServer dnsServer;
ESP8266WebServer webServer(80);

void setup() {
	Serial.begin(115200);
	EEPROM.begin(512);
	delay(10);
	if (restoreConfig()) {
		if (checkConnection()) {
			settingMode = false;
			startWebServer();
			return;
		}
	}
	settingMode = true;
	setupMode();

}


void loop() {
	uint8_t elapsedMinutes;

	if (settingMode) {
		dnsServer.processNextRequest();
	}
	webServer.handleClient();

	if (chronoConnection.hasPassed(30))
	{
		TestConnection();
		chronoConnection.restart();
	}

	if (chronoSendData.hasPassed(30))
	{
		sendDataToServer();
		chronoSendData.restart();
	}


}


void sendDataToServer()
{
	const String Host = "http://temp.giacob.be";
	const String tag = "/Frigo1";
	int temp;
	String s = "";



	HTTPClient http;

	randomSeed(analogRead(0));  //inizializzo il generatore di valori casuali

	temp = random(-20, 10);  //min, max

	s += Host;
	s += tag;
	s += "/" + String(temp);

	http.begin(s);
	{
		Serial.println("Sending data: " + s + " °C | Server response: ");

		int httpCode = http.GET();
		if (httpCode > 0) {
			// HTTP header has been send and Server response header has been handled
			Serial.printf("  [HTTP] GET... code: %d\n", httpCode);
		}
		else {
			Serial.printf("  [HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
		}
	}
	http.end();

}


void TestConnection() {
	const int led5 = 5;
	int s;
	bool ConnectionStatus;

	pinMode(led5, OUTPUT);

	for (int i = 0; i < 10; i++) {
		digitalWrite(led5, LOW);
		delay(100);
		digitalWrite(led5, HIGH);
		delay(100);
	}
	if (WiFi.status() == WL_CONNECTED) {
		digitalWrite(led5, LOW);
	}

}

boolean restoreConfig() {
	Serial.println("Reading EEPROM...");
	String ssid = "";
	String pass = "";
	if (EEPROM.read(0) != 0) {
		for (int i = 0; i < 32; ++i) {
			ssid += char(EEPROM.read(i));
		}
		Serial.print("SSID: ");
		Serial.println(ssid);
		for (int i = 32; i < 96; ++i) {
			pass += char(EEPROM.read(i));
		}
		Serial.print("Password: ");
		Serial.println(pass);
		WiFi.begin(ssid.c_str(), pass.c_str());
		return true;
	}
	else {
		Serial.println("Config not found.");
		return false;
	}
}

boolean checkConnection() {
	int count = 0;
	Serial.print("Waiting for Wi-Fi connection");
	while (count < 30) {
		if (WiFi.status() == WL_CONNECTED) {
			Serial.println();
			Serial.println("Connected!");
			return (true);
		}
		delay(500);
		Serial.print(".");
		count++;
	}
	Serial.println("Timed out.");
	return false;
}

void startWebServer() {
	if (settingMode) {
		Serial.print("Starting Web Server at ");
		Serial.println(WiFi.softAPIP());
		webServer.on("/settings", []() {
			String s = "<h1>Wi-Fi Settings</h1><p>Please enter your password by selecting the SSID.</p>";
			s += "<form method=\"get\" action=\"setap\"><label>SSID: </label><select name=\"ssid\">";
			s += ssidList;
			s += "</select><br>Password: <input name=\"pass\" length=64 type=\"password\"><input type=\"submit\"></form>";
			webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
		});
		webServer.on("/setap", []() {
			for (int i = 0; i < 96; ++i) {
				EEPROM.write(i, 0);
			}
			String ssid = urlDecode(webServer.arg("ssid"));
			Serial.print("SSID: ");
			Serial.println(ssid);
			String pass = urlDecode(webServer.arg("pass"));
			Serial.print("Password: ");
			Serial.println(pass);
			Serial.println("Writing SSID to EEPROM...");
			for (int i = 0; i < ssid.length(); ++i) {
				EEPROM.write(i, ssid[i]);
			}
			Serial.println("Writing Password to EEPROM...");
			for (int i = 0; i < pass.length(); ++i) {
				EEPROM.write(32 + i, pass[i]);
			}
			EEPROM.commit();
			Serial.println("Write EEPROM done!");
			String s = "<h1>Setup complete.</h1><p>device will be connected to \"";
			s += ssid;
			s += "\" after the restart.";
			webServer.send(200, "text/html", makePage("Wi-Fi Settings", s));
			ESP.restart();
		});
		webServer.onNotFound([]() {
			String s = "<h1>AP mode</h1><p><a href=\"/settings\">Wi-Fi Settings</a></p>";
			webServer.send(200, "text/html", makePage("AP mode", s));
		});
	}
	else {
		Serial.print("Starting Web Server at ");
		Serial.println(WiFi.localIP());
		webServer.on("/", []() {
			String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset Wi-Fi Settings</a></p>";
			webServer.send(200, "text/html", makePage("STA mode", s));
		});
		webServer.on("/reset", []() {
			for (int i = 0; i < 96; ++i) {
				EEPROM.write(i, 0);
			}
			EEPROM.commit();
			String s = "<h1>Wi-Fi settings was reset.</h1><p>Please reset device.</p>";
			webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", s));
		});
	}
	webServer.begin();
}

void setupMode() {
	WiFi.mode(WIFI_STA);
	if (WiFi.disconnect()) {     //if added by AM 
		delay(100);
		Serial.println("Scan networks");
		int n = WiFi.scanNetworks();
		delay(100);
		for (int i = 0; i < n; ++i) {
			ssidList += "<option value=\"";
			ssidList += WiFi.SSID(i);
			ssidList += "\">";
			ssidList += WiFi.SSID(i);
			ssidList += "</option>";
		}
		Serial.println("Networks: " + ssidList);
		delay(100);
		WiFi.mode(WIFI_AP);
		WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
		WiFi.softAP(apSSID);
		dnsServer.start(53, "*", apIP);
		startWebServer();
		Serial.print("Starting Access Point at \"");
		Serial.print(apSSID);
		Serial.println("\"");
	}
	else
		Serial.println("Sconnessione dalla rete WiFi non riuscita");
}

String makePage(String title, String contents) {
	String s = "<!DOCTYPE html><html><head>";
	s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\">";
	s += "<title>";
	s += title;
	s += "</title></head><body>";
	s += contents;
	s += "</body></html>";
	return s;
}

String urlDecode(String input) {
	String s = input;
	s.replace("%20", " ");
	s.replace("+", " ");
	s.replace("%21", "!");
	s.replace("%22", "\"");
	s.replace("%23", "#");
	s.replace("%24", "$");
	s.replace("%25", "%");
	s.replace("%26", "&");
	s.replace("%27", "\'");
	s.replace("%28", "(");
	s.replace("%29", ")");
	s.replace("%30", "*");
	s.replace("%31", "+");
	s.replace("%2C", ",");
	s.replace("%2E", ".");
	s.replace("%2F", "/");
	s.replace("%2C", ",");
	s.replace("%3A", ":");
	s.replace("%3A", ";");
	s.replace("%3C", "<");
	s.replace("%3D", "=");
	s.replace("%3E", ">");
	s.replace("%3F", "?");
	s.replace("%40", "@");
	s.replace("%5B", "[");
	s.replace("%5C", "\\");
	s.replace("%5D", "]");
	s.replace("%5E", "^");
	s.replace("%5F", "-");
	s.replace("%60", "`");
	return s;
}
