#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <EEPROM.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPClient.h>
#include <TimeLib.h>
#include <Chrono.h>
#include <OneWire.h>
#include <DallasTemperature.h>;
#include <ESP8266WebServer.h>
#include "jsonConfig.h"
#include "alertMail.h"
//#include "ThingSpeak.h"


extern "C" {
#include "user_interface.h"
}


//Variables for webserver
ESP8266WebServer webServer(80);
const IPAddress apIP(192, 168, 1, 1);
const char* apSSID = "ESP8266_SETUP";
const char* apPSW = "arcinBOLDO321";
const int MAX_Sensors = 4;
String ssidList;
boolean settingMode;

//Variables for the configuration file
const char *configFilename = "/config.json";
Config mainConfig;
SensorData sensorsData[MAX_Sensors];

//Variables for time reading interval
Chrono chrono0(Chrono::SECONDS);
Chrono chrono1(Chrono::SECONDS);
Chrono chrono2(Chrono::SECONDS);
Chrono chrono3(Chrono::SECONDS);



DNSServer dnsServer;

// Data wire is conntec to the Arduino digital pin 4
#define ONE_WIRE_BUS 4 //GPIO pin 4

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

void LED_ERROR() {
	const int led5 = 5;
	int n = 100;
	do
	{
		digitalWrite(led5, LOW);
		delay(30);
		digitalWrite(led5, HIGH);
		delay(30);
		
	} while (n-- == 0);

}

void setup() {
	Serial.begin(115200);
	EEPROM.begin(512);
	delay(10);

	if (restoreConfig()) {

		int cks = checkSensors();
		switch (cks)
		{
		case -1: 
			Serial.println(F("ATTENZIONE:Sono stati rimossi uno o più sensori rispetto alla configurazione precedente"));
			Serial.println(F("Verificare le impostazioni."));
			break;
		case 1:
			Serial.println(F("ATTENZIONE:Sono stati aggiunti dei sensori rispetto alla configurazione precedente"));
			Serial.println(F("Entrare nelle impostazioni per configurarli."));
			break;
		case 0:
			Serial.println("ATTENZIONE: NON sono stati rilevati sensori. Il dispositivo è bloccato");
			Serial.println("Spegnere il dispositivo, collegare almeno un sensore e configurarlo.");
			break;
		}

		if (cks = 2 ) {
			
			//carico le informazioni dei sensori
			for (int i = 0; i < MAX_Sensors; i++)
				sensorsData[i] = getSensorInfo(i);

			if (checkConnection()) {
				settingMode = false;
				startWebServer();
				sensors.begin();
				return;
			}
		}
		settingMode = true;
		setupMode();
	}
	else 
		LED_ERROR();
}

int checkSensors() {
	/*
	Return  2 = OK
	Return -1 = Sono stati scollegati uno o più sensori rispetto alla configurazione precedente
	Return  1 = Sono stati aggiunti dei sensori rispetto alla configurazione precedente
	Return  0 = Non ci sono sensori
	*/
	bool check = false;
	int sensorsInstalled = int(sensors.getDeviceCount());
	
	check = (sensorsInstalled = mainConfig.sensorCount) && (sensorsInstalled > 0); //nessuna variazione e almeno un sensore collegato

	if (check) 
		return 2; //ok
	else {

		if (sensorsInstalled = 0)	//Nessun sensore collegato
			return 0;
		else {
			if (mainConfig.sensorCount < sensorsInstalled)  //Sono stati collelgati uno o più sensori
				return 1;
			else
				return -1;									//Sono stati tolti uno o più sensori
		}

	}
	
}

int parseInput(String input) {
	bool found = false;

	if (input == "") {
		found = true;
		return -1;
	}

	if (input == "ip") {
		found = true;
		return 1;
	}

	if (input == "read") {
		found = true;
		return 2;
	}

	if (input == "config") {
		found = true;
		return 3;
	}
	
	if (input == "test") {
		found = true;
		return 4;
	}

	if (!found) return 0;
}

SensorData getSensorInfo(int id) {
	SensorData s;
	s.email = mainConfig.email;
	s.id = 0;
	s.name = mainConfig.sensors[id];
	s.tempMAX = mainConfig.tempMAXs[id];
	s.tempMIN = mainConfig.tempMINs[id];
	s.timeInterval = mainConfig.timeIntervals[id];
	s._prevTemp = -INT_MAX;
	s._lastTemp = -INT_MAX;
	s.config = &mainConfig;
	return s;
}


void loop() {
	uint8_t elapsedMinutes;

	String r = Serial.readString();
	String n;

	switch (parseInput(r)) {

	case 1:
		Serial.print("IP: ");
		Serial.println(WiFi.localIP());
		break;

	case 2:
		
		Serial.println("Letture:");
		sensors.requestTemperatures();
		delay(500);
		for (int i = 0; i < MAX_Sensors; i++)
		{
			Serial.println("Sensore " + String(i) + " >>>>> " + String(readTemperature(i)) + "°C");
			
		}

		break;
	case 3:
		printConfig(mainConfig, MAX_Sensors);
		break;

	case 4:
		for (int i = 0; i < MAX_Sensors; i++)
		{
			sendDataToServer(i);
		}
		
		break;

	case 0: Serial.println(r + " è un comando non valido. Usa 'ip', 'read', 'test', 'config'.");
	}


	if (settingMode) {
		dnsServer.processNextRequest();
	}
	webServer.handleClient();


	if (TestConnection()) {
		// Call sensors.requestTemperatures() to issue a global temperature and Requests to all devices on the bus
		sensors.requestTemperatures();

		if (chrono0.hasPassed(mainConfig.timeIntervals[0] * 60)) {
			chrono0.restart();
			sendDataToServer(0);
		}

		if (chrono1.hasPassed(mainConfig.timeIntervals[1] * 60)) {
			chrono1.restart();
			sendDataToServer(1);
		}

		if (chrono2.hasPassed(mainConfig.timeIntervals[2] * 60)) {
			chrono2.restart();
			sendDataToServer(2);
		}

		if (chrono3.hasPassed(mainConfig.timeIntervals[3] * 60)) {
			chrono3.restart();
			sendDataToServer(3);
		}
	}



}

float readTemperature(int sensorId) {
	return  sensors.getTempCByIndex(sensorId); // Why "byIndex"? You can have more than one IC on the same bus. 0 refers to the first IC on the wire
}

void sendTempToServer(int sensorId) {
	const String Host = "http://temp.giacob.be";
	const String tag = sensorsData[sensorId].name;
	String s = "";

	HTTPClient http;
	
	s += Host;
	s += "/" + tag;
	s += "/" + String(sensorsData[sensorId]._lastTemp);

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

void sendDataToServer(int sensorID)
{
	float temp;

	sensorsData[sensorID]._prevTemp = sensorsData[sensorID]._lastTemp;
	sensorsData[sensorID]._lastTemp = readTemperature(sensorsData[sensorID].id);

	sendTempToServer(sensorID);
	
	//ThingSpeakSendData(temp, mainConfig);

	if (!checkTemperature(sensorsData[sensorID])) {
		sendAlert(sensorsData[sensorID]);
	}

}


bool TestConnection() {
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
		return true;
	}
	else
		return false;
}

boolean restoreConfig() {
	bool wifiConfigurationRestored = false;
	bool fileConfigurationRestored = false;


	Serial.println("Reading EEPROM...");
	String ssid = "";
	String pass = "";

	if (EEPROM.read(0) != 0) {
		for (int i = 0; i < 32; ++i) {
			ssid += char(EEPROM.read(i));
		}

		for (int i = 32; i < 96; ++i) {
			pass += char(EEPROM.read(i));
		}
		Serial.println("Loaded WiFi configuration for " + ssid);
		WiFi.begin(ssid.c_str(), pass.c_str());
		wifiConfigurationRestored = true;
	}
	else {
		Serial.println("Wifi configuration not found.");
	}

	SPIFFStart();
	if (!wifiConfigurationRestored) {
		return false;
	}
	else
	{
		if (checkFileConfig(configFilename, MAX_Sensors)) {
			mainConfig = loadConfig(configFilename, MAX_Sensors);
			return true;
		}
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
		delay(200);
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
			webServer.send(200, "text/html", makeHTMLSettingPage(ssidList));
		});

		webServer.on("/setap", []() {
			//cancello le informazioni WiFi dalla EPROM
			for (int i = 0; i < 96; ++i) {
				EEPROM.write(i, 0);
			}
			//Scrivo le nuove informazioni sulla rete Wifi
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


			Serial.println("Writing configuration to file");

			//Nome cliente
			String cliente = urlDecode(webServer.arg("idcli"));
			mainConfig.clientID = cliente;
			Serial.println("Cliente " + mainConfig.clientID);

			//Nomi dei sensori
			Serial.println("Sensori:");
			for (int i = 0; i < MAX_Sensors; i++) {
				String sensore = urlDecode(webServer.arg("sensor" + String(i)));
				mainConfig.sensors[i] = sensore;
				Serial.println("Sensore " + String(i) + ": " + mainConfig.sensors[i]);
			}

			//Tempo di intervallo per le letture
			for (int i = 0; i < MAX_Sensors; i++) {
				String aValue = urlDecode(webServer.arg("time" + String(i)));
				float interval = aValue.toFloat();
				mainConfig.timeIntervals[i] = interval;
				Serial.print("Sensore [" + mainConfig.clientID + "]: ");
				Serial.println("Intervallo letture "  + aValue + " minuti");
			}

			//Temperatura minima per ogni sensore
			for (int i = 0; i < MAX_Sensors; i++)
			{
				String aValue = urlDecode(webServer.arg("mintemp" + String(i)));
				float tMin = aValue.toFloat();
				mainConfig.tempMINs[i]= tMin;
				Serial.print("Sensore [" + mainConfig.clientID + "]: ");
				Serial.println("Temperatura minima: " + aValue + " °C");
			}

			//Temperatura massima per ogni sensore
			for (int i = 0; i < MAX_Sensors; i++)
			{
				String aValue = urlDecode(webServer.arg("maxtemp" + String(i)));
				float tMax = aValue.toFloat();
				mainConfig.tempMAXs[i] = tMax;
				Serial.print("Sensore [" + mainConfig.clientID + "]: ");
				Serial.println("Temperatura massima: " + aValue + " °C");
			}

			//email per inviare segnalazioni
			String email = urlDecode(webServer.arg("email"));
			mainConfig.email = email;
			Serial.println("Email " + mainConfig.email);


			//Numero di errori consecutivi in cui inviare la segnalazione
			String err = urlDecode(webServer.arg("errors"));
			mainConfig.errors = err.toInt();
			Serial.println("Errori consecutivi " + err);

			//Salvataggio
			if (saveConfig(mainConfig, configFilename, MAX_Sensors)) {
				Serial.println("File configuration saved;");
				printConfig(mainConfig, MAX_Sensors);
			}

			String s = "<h1>Impostazione completata.</h1><p>Il dispositivo si connetterà a \"";
			s += ssid;
			s += "\" dopo il riavvio.";
			webServer.send(200, "text/html", makePage("Impostazioni", "", s));
		});

		webServer.onNotFound([]() {
			String s = "<h1>AP mode</h1><p><a href=\"/settings\">Impostazioni</a></p>";
			webServer.send(200, "text/html", makePage("AP mode", "", s));
		});
	}
	else {
		Serial.print("Starting Web Server at ");
		Serial.println(WiFi.localIP());
		webServer.on("/resetpage", []() {
			String s = "<h1>STA mode</h1><p><a href=\"/reset\">Reset impostazioni</a></p>";
			webServer.send(200, "text/html", makePage("STA mode", "", s));
		});

		webServer.on("/reset", []() {
			for (int i = 0; i < 96; ++i) {
				EEPROM.write(i, 0);
			}
			EEPROM.commit();
			String s = "<h1>Impostazioni Wi-Fi resetate.</h1><p>Riavviare il dispositivo.</p>";
			webServer.send(200, "text/html", makePage("Reset Wi-Fi Settings", "", s));
		});
	}
	webServer.begin();
}


void setupMode() {
	Serial.begin(115200); /////////Cambiare in uno più veloce

	//https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/scan-examples.html?highlight=wifi%20scannetworks
	Serial.println("Starting WiFi STAtion Mode");
	WiFi.mode(WIFI_STA);
	WiFi.disconnect();
	delay(100);


	Serial.println("Getting networks...");
	int n = WiFi.scanNetworks();
	delay(100);
	for (int i = 0; i < n; ++i) {
		ssidList += "<option value=\"";
		ssidList += WiFi.SSID(i);
		ssidList += "\">";
		ssidList += WiFi.SSID(i);
		ssidList += "</option>";
	}
	Serial.println(ssidList);
	Serial.println("");

	delay(100);
	Serial.println("Starting WiFi Access Point Mode");
	//WiFi.mode(WIFI_AP);

	IPAddress local_IP(192, 168, 4, 1);  //Default
	IPAddress gateway(192, 168, 4, 9);
	IPAddress subnet(255, 255, 255, 0);
	const bool hidenSSID = true;
	Serial.print("Setting soft-AP configuration ... ");
	Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");

	Serial.print("Setting soft-AP ... ");
	Serial.println(WiFi.softAP(apSSID, apPSW, 8, !hidenSSID) ? "Ready" : "Failed!");
	dnsServer.start(53, "*", apIP);
	startWebServer();
	WiFi.printDiag(Serial);
	Serial.println("");
	Serial.print("Starting Access Point at \"");
	Serial.print(apSSID);
	Serial.println("\"");
}

String makePage(String title, String style, String contents) {
	String s = "<!DOCTYPE html><html lang=\"it\" xmlns=\"http://www.w3.org/1999/xhtml\"><html><head>";
	s += "<meta name=\"viewport\" content=\"width=device-width,user-scalable=0\" charset=\"utf-8\" >";
	s += "<title>";
	s += title;
	s += "</title>\n" + style + "</head><body>";
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

String makeHTMLSettingPage(const String ssidList) {

	Config localConfig = loadConfig(configFilename, MAX_Sensors);

	String style = "	<style type=\"text/css\">";
	style += "		form {";
	style += "			width: 300px;";
	style += "			margin-left: 10px;";
	style += "			padding: 10px 0;";
	style += "			border: 3px double #99a;";
	style += "		}";
	style += "		.data{";
	style += "			margin-bottom:15px;";
	style += "			margin-left: 5px;";
	style += "		}";
	style += "		label,#submit {";
	style += "			margin-left: 5px;";
	style += "          margin-top:15px;";
	style += "		}";
	style += "	</style>";

	String webpage = "	<h1>Configurazione rilevatore temperature</h1>";
	webpage += "	<form method=\"get\" action=\"setap\">";

	webpage += "	<fieldset>";
	webpage += "     <legend>Wi-Fi</legend>";
	webpage += "		<label class=\"data\">SSID: </label><select name=\"ssid\">";
	webpage += ssidList;
	webpage += "		</select><br />";
	webpage += "		<label>Password: </label><br />";
	webpage += "		<input class=\"data\" name=\"pass\" maxlength=64 type=\"password\"><br />";
	webpage += "    </fieldset>";

	webpage += "	<fieldset>";
	webpage += "     <legend>Cliente</legend>";
	webpage += "		<label>Nome</label><br />";
	webpage += "		<input class=\"data\" name=\"idcli\" maxlength=\"10\" type=\"text\" value=\"" + localConfig.clientID + "\"/><br />";
	webpage += "		<label>e-mail</label><br />";
	webpage += "		<input class=\"data\" name=\"email\" type=\"email\" value=\"" + String(localConfig.email) + "\"/><br />";
	webpage += "    </fieldset>";

	webpage += "	<fieldset>";
	webpage += "     <legend>Generale</legend>";
	webpage += "		<label>Errori consecutivi</label><br />";
	webpage += "		<input class=\"data\" name=\"errors\" type=\"number\" value=\"" + String(localConfig.errors) + "\"/><br />";
	webpage += "    </fieldset>";

	for (int i = 0; i < MAX_Sensors; i++)
	{
		String name = "sensor" + String(i);

		(i > (localConfig.sensorCount - 1)) ? 
			webpage += "	<fieldset disabled=\"disabled\">" : 
			webpage += "	<fieldset>";
		
		webpage += "     <legend>" + name + "</legend>";
		webpage += "		<label>Nome</label><br />";
		(i>(localConfig.sensorCount - 1)) ?
			webpage += "		<input class=\"data\" name=\"" + name + "\" maxlength=\"10\" type=\"text\" value=\"\" /><br />" :
			webpage += "		<input class=\"data\" name=\"" + name + "\" maxlength=\"10\" type=\"text\" value=\"" + localConfig.sensors[i] + "\"/><br />";

		name = "time" + String(i);
		webpage += "     <label>Intervallo letture, in minuti</label><br />";
		(i>(localConfig.sensorCount - 1)) ?
			webpage += "		<input class=\"data\" name=\"" + name + "\" type=\"number\" value=\"\" /><br />" :
			webpage += "		<input class=\"data\" name=\"" + name + "\" type=\"number\" value=\"" + String(localConfig.timeIntervals[i]) + "\"/><br />";

		name = "mintemp" + String(i);
		webpage += "     <label>Temperatura minima</label><br />";
		(i>(localConfig.sensorCount - 1)) ?
			webpage += "		<input class=\"data\" name=\"" + name + "\" type=\"number\" value=\"\" /><br />" :
			webpage += "		<input class=\"data\" name=\"" + name + "\" type=\"number\" value=\"" + String(localConfig.tempMINs[i]) + "\"/><br />";

		name = "maxtemp" + String(i);
		webpage += "     <label>Temperatura massima</label><br />";
		(i>(localConfig.sensorCount - 1)) ?
			webpage += "		<input class=\"data\" name=\"" + name + "\" type=\"number\" value=\"\" /><br />" :
			webpage += "		<input class=\"data\" name=\"" + name + "\" type=\"number\" value=\"" + String(localConfig.tempMINs[i]) + "\"/><br />";

	}
	

	webpage += "		<input id=\"submit\" type=\"submit\" value=\"Salva\" />";
	webpage += "	</form>";


	//AGGIUNGERE I LCAMPO ERRORS
	return makePage("ESP8266 Configuratore", style, webpage);
}


