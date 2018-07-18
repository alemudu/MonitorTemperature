// ThingSpeak.h

#ifndef _THINGSPEAK_h
#define _THINGSPEAK_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#endif

#include "jsonConfig.h";
#include <WiFiClient.h>

void ThingSpeakSendData(float temp, Config config);
