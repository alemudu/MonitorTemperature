// jsonConfig.h
/*
{
"clientID": "abcdefghij",
"sensor0": "abcdefghij",
"sensor1": "abcdefghij",
"sensor2": "abcdefghij",
"sensor3": "abcdefghij",
"timeInterval": 1351824120,
"tempMIN": -100,
"tempMAX": 100,
"email": "mdmanutenzioni@gmail.com",
"errors": 2
}
PARSING: JSON_OBJECT_SIZE(10) + 210 from https://arduinojson.org/v5/assistant/
{
	"clientID": "abcdefghij",
	"email": "mdmanutenzioni@gmail.com",
	"errors": 2,
	"sensorsCount": 4,
	"sensors": [
		{
			"id": 0,
			"nome": "abcdefghij",
			"tempMIN": -100,
			"tempMAX": 100,
			"timeInterval": 1351824120
		},
		{
			"id": 1,
			"nome": "abcdefghij",
			"tempMIN": -100,
			"tempMAX": 100,
			"timeInterval": 1351824120
		},
		{
			"id": 2,
			"nome": "abcdefghij",
			"tempMIN": -100,
			"tempMAX": 100,
			"timeInterval": 1351824120
		},
		{
			"id": 3,
			"nome": "abcdefghij",
			"tempMIN": -100,
			"tempMAX": 100,
			"timeInterval": 1351824120
		}
	]
}
PARSING: JSON_ARRAY_SIZE(4) + 5*JSON_OBJECT_SIZE(5) + 410;;
*/

#ifndef _JSONCONFIG_h
#define _JSONCONFIG_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif

struct Config {
  String clientID;		//name of customer max 10 chars
  String sensors[4];	//array of sensors names
  float tempMINs[4];		//array of temperature min
  float tempMAXs[4];		//array of temperature max
  float timeIntervals[4]; //time interval between each read
  String email;			//email for errors
  int errors;			//numero di errori consecutivi in cui scatta la mail
  int sensorCount;		//FOR INTERNAL USE
};

struct SensorData {
	int id;
	String name;
	float tempMIN;
	float tempMAX;
	float timeInterval;
	String email;
	float _lastTemp;
	float _prevTemp;
	Config *config;
};

#endif


Config loadConfig(const char *filename, const int MAX_Sensors);

bool saveConfig(const Config aConfig, const char *filename, const int MAX_Sensors);

void printConfig(Config aConfig, const int MAX_Sensors);

bool checkFileConfig(const char *filename, const int MAX_Sensors);

void SPIFFStart();
