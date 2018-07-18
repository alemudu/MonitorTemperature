// alertMail.h

#ifndef _ALERTMAIL_h
#define _ALERTMAIL_h

#if defined(ARDUINO) && ARDUINO >= 100
	#include "arduino.h"
#else
	#include "WProgram.h"
#endif


#endif


#include "jsonConfig.h";
#include <WiFiClient.h>


bool checkTemperature(SensorData sensor);
void sendAlert(SensorData sensor);