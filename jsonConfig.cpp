#include "ArduinoJson.h"
#include "jsonConfig.h"
#include "FS.h"

void SPIFFStart() {
	SPIFFS.begin();
	Serial.println("SPIFFS.begin()");
}

bool checkFileConfig(const char *filename, const int MAX_Sensors) {
	bool fileExists;
	bool configChecked = false;
	Config myConfig;

	Serial.println(F("***checkFileConfig procedure"));
	fileExists = SPIFFS.exists(filename);
	if (fileExists) {
		Serial.println(F("  file found"));
		myConfig = loadConfig(filename, MAX_Sensors);

		configChecked = (myConfig.clientID != "") && (myConfig.sensors[0] != "");

		for (int i = 0; i < myConfig.sensorCount; i++) //controllo solo i sensori installati
		{
			configChecked = configChecked && (myConfig.timeIntervals[i] > 0) && (myConfig.tempMINs[i] != myConfig.tempMAXs[i]);
		}

		(configChecked) ? Serial.println(F("  configurazion OK")) : Serial.println(F("  configurazion BUD:"));

	}
	else
		Serial.println(F("  File not found"));

	Serial.println(F("***END checkFileConfig procedure"));
	return configChecked;
}

Config loadConfig(const char *filename, const int MAX_Sensors) {
	const size_t bufferSize = JSON_ARRAY_SIZE(4) + 5 * JSON_OBJECT_SIZE(5) + 410;
	Config loadedConfig;

	Serial.println("  Searchiing for configuration file ...");
	if (SPIFFS.exists(filename))
	{
		File configFile = SPIFFS.open(filename, "r");
		if (configFile)
			Serial.println("  Configuration file found and opened for read");

		size_t size = configFile.size();
		Serial.println("  File size:" + size);
		if (size = 0) {
			Serial.println("  ATTENTION: Configuration file empty or corrupted!");
		}
		else {

			// Allocate the memory pool on the stack.
			// Don't forget to change the capacity to match your JSON document.
			// Used https://arduinojson.org/v5/assistant/ to compute the capacity.
			// See also https://arduinojson.org/v5/faq/what-are-the-differences-between-staticjsonbuffer-and-dynamicjsonbuffer/
			const size_t bufferSize = JSON_ARRAY_SIZE(4) + 5 * JSON_OBJECT_SIZE(5) + 410;
			StaticJsonDocument<bufferSize> jsonBuffer;

			Serial.print("  Deserializing configuration file to buffer...");
			DeserializationError error = deserializeJson(jsonBuffer, configFile);					//https://arduinojson.org/v6/example/config/
			if (error)
				Serial.println(F("  Fails: Is impossible read from the configuration file for serialize it."));
			else
				Serial.println("  Success.");

			JsonObject &jsonRoot = jsonBuffer.as<JsonObject>();							//get the Document jsonRoot
			
			loadedConfig.clientID = jsonRoot["clientID"].as<String>();
			loadedConfig.email= jsonRoot["email"].as<String>();
			loadedConfig.errors = jsonRoot["errors"];
			loadedConfig.sensorCount = jsonRoot["sensorsCount"];

			JsonArray &jSensors = jsonRoot["sensors"].as<JsonArray>();	//Get the array of sensors
			for (int i = 0; i < MAX_Sensors; i++)						//Parse the array of sensors
			{
				JsonObject &jSensor = jSensors[i].as<JsonObject>();
				loadedConfig.sensors[i] = jSensor["name"].as<String>();
				loadedConfig.tempMINs[i] = jSensor["tempMIN"];
				loadedConfig.tempMAXs[i] = jSensor["tempMAX"];
				loadedConfig.timeIntervals[i] = jSensor["timeInterval"];
			}

			printConfig(loadedConfig, MAX_Sensors);
			Serial.println("***END loadConfig procedure");

		}
		configFile.close();
	}
	else {
		Serial.printf("ERROR: %s configuration file not found!", &filename);
	}

	return loadedConfig;
}

bool saveConfig(const Config aConfig, const char *filename, const int MAX_Sensors) {
	Serial.println("***SaveConfig procedure");
	if (SPIFFS.exists(filename)) {
		SPIFFS.remove(filename);
		Serial.println("file removed");
	}

	File configFile = SPIFFS.open(filename, "w");
	if (!configFile) {
		Serial.println("  Impossibile creare il file di configurazione");
	}
	else {
		Serial.println("  File di configurazione creato");

		const size_t bufferSize = JSON_ARRAY_SIZE(4) + 5 * JSON_OBJECT_SIZE(5) + 410;
		StaticJsonDocument<bufferSize> jsonBuffer;

		JsonObject &jsonRoot = jsonBuffer.to<JsonObject>();			//https://arduinojson.org/v6/api/json/serializejson/

		jsonRoot["clientID"] = aConfig.clientID;					//Nome cliente
		jsonRoot["email"] = aConfig.email;							//EMail	
		jsonRoot["errors"] = aConfig.errors;						//Errori
		jsonRoot["sensorsCount"] = aConfig.sensorCount;				//Numero di sensori installati

		JsonArray &jSensors = jsonRoot.createNestedArray("sensors");
		for (int i = 0; i < MAX_Sensors; i++)						//Per ogni sensore...
		{
			
			JsonObject &jSensor = jSensors.createNestedObject();
			jSensor["id"] = i;										//id
			jSensor["name"] = aConfig.sensors[i];					//nome
			jSensor["tempMIN"] = aConfig.tempMINs[i];				//temperature minima
			jSensor["tempMAX"] = aConfig.tempMAXs[i];				//temperatura massima
			jSensor["timeInterval"] = aConfig.timeIntervals[i];		//intervallo di tempo tra due letture
		}



		// Serialize JSON to file
		if (serializeJson(jsonRoot, configFile) == 0) {
			Serial.println(F("  Failed to write to file"));
		}
		else Serial.println("  File salvato correttamente");

	}
	configFile.close();
	Serial.println(" File di configurazione chiuso");
	Serial.println("***END SaveConfig procedure");
}

void printConfig(Config aConfig, const int MAX_Sensors) {
	Serial.println();
	Serial.println("Printing configuration:");
	Serial.println("  Cliente: " + aConfig.clientID);
	Serial.println("  Email: " + aConfig.email);
	Serial.print("  Errori consecutivi: ");  Serial.println(String(aConfig.errors));
	Serial.print("  Sensori collegati: ");	 Serial.println(String(aConfig.sensorCount));

	for (int i = 0; i < MAX_Sensors; i++)
	{
		Serial.println("  Sensore" + String(i)+": ");
		Serial.println("	Id: " + String(i));
		Serial.println("	Nome: " + aConfig.sensors[i]);
		Serial.println("	Temperatura MIN: " + String(aConfig.tempMINs[i]) + " °C");
		Serial.println("	Temperatura MAX: " + String(aConfig.tempMAXs[i]) + " °C");
		Serial.println("	Intervallo letture: " + String(aConfig.timeIntervals[i]) + " minuti");
	}

	Serial.println("");
	
	Serial.println("End configuration");
	Serial.println();
  Serial.println();
}

