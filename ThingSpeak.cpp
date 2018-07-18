// 
// 
// 
#include "ThingSpeak.h"


WiFiClient clientThingSpeak;
const String apiKey = "L2VT5GZXMGX4XMI5";
const String serverThingSpeak = "api.thingspeak.com";


bool checkTimeutTS() {
	int loopCount = 0;


	while (!clientThingSpeak.available()) {
		delay(1);
		loopCount++;

		// if nothing received for 10 seconds, timeout
		if (loopCount > 10000) {
			clientThingSpeak.stop();
			Serial.println(F("\r\nTimeout"));
			return 0;
		}
	}

	return 1;
}



void getResponseTS() {
	String line;

	while (clientThingSpeak.available()) {
		line = clientThingSpeak.readStringUntil('\n');
		Serial.println(line);
	}
}



void efailTS()
{
	byte thisByte = 0;
	int loopCount = 0;

	clientThingSpeak.println(F("QUIT"));

	if (!checkTimeutTS()) return;

	getResponseTS();

	clientThingSpeak.stop();

	Serial.println(F("disconnected"));
}




byte eRcvTS()
{
	byte respCode;
	byte thisByte;

	if (!checkTimeutTS()) return 0;

	respCode = clientThingSpeak.peek();

	getResponseTS();

	if (respCode >= '4')
	{
		efailTS();
		return 0;
	}

	return 1;
}


void ThingSpeakSendData(float temp, Config myConfig)
{
	
  String channelID = myConfig.clientID;
  String jsonReq = "channels/"+channelID+"/bulk_update.json";

	if (!clientThingSpeak.connected()) {
		clientThingSpeak.connect(serverThingSpeak, 80);
	}
	if (clientThingSpeak.connected()) 
		Serial.println("Connected to " + serverThingSpeak);
	else
		Serial.println("Not Connected to " + serverThingSpeak);

	String postData = "&field1=";
      	postData += String(temp);
      	postData += "&field2=";
      	postData += String(temp*1.5);


	clientThingSpeak.println("POST /update HTTP/1.1");
	clientThingSpeak.println("Host: "+ serverThingSpeak);
	clientThingSpeak.println("User-Agent: ESP8266 (nothans)/1.0");
	clientThingSpeak.println("Connection: close");
	clientThingSpeak.println("X-THINGSPEAKAPIKEY: " + apiKey );
	clientThingSpeak.println("Content-Type: application/x-www-form-urlencoded");
	clientThingSpeak.println("Content-Length: " + String(postData.length()));
	clientThingSpeak.println("");
	clientThingSpeak.print(postData);

	Serial.println("Send: " + postData);

	clientThingSpeak.stop();
}
