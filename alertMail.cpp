/*
 Name:		alertMail.cpp
 Created:	11/07/2018 14:09:53
 Author:	alemu
 Editor:	http://www.visualmicro.com
*/


#include "alertMail.h"
#include <Base64.h>;


WiFiClient clientSMTP2GO;
base64 _base64;

bool checkValue(float value, SensorData sensor) {
	return ((value >= sensor.tempMIN) && (value <= sensor.tempMAX));
}

bool checkTemperature(SensorData sensor){
	return (checkValue(sensor._lastTemp, sensor)) || (checkValue(sensor._prevTemp, sensor));
}




bool checkTimeut() {
  int loopCount = 0;


  while (!clientSMTP2GO.available()) {
    delay(1);
    loopCount++;

    // if nothing received for 10 seconds, timeout
    if (loopCount > 10000) {
      clientSMTP2GO.stop();
      Serial.println(F("\r\nTimeout"));
      return 0;
    }
  }

  return 1;
}



void getResponse() {
  String line;

  while (clientSMTP2GO.available()) {
    line = clientSMTP2GO.readStringUntil('\n');
    Serial.println(line);
  }
}



void efail()
{
  byte thisByte = 0;
  int loopCount = 0;

  clientSMTP2GO.println(F("QUIT"));

  if (!checkTimeut()) return;

  getResponse();

  clientSMTP2GO.stop();

  Serial.println(F("disconnected"));
}




byte eRcv()
{
  byte respCode;
  byte thisByte;


  if (!checkTimeut()) return 0;

  respCode = clientSMTP2GO.peek();

  getResponse();

  if (respCode >= '4')
  {
    efail();
    return 0;
  }

  return 1;
}




void sendAlert(SensorData sensor) {
	const String emailDomain = "@gestionetemperature.com";
	const String commonPsw = "aDBweWV1cm11cm0wMHNpMDJuY21uMnIw";
	const String cmdEHLO = "EHLO";
	const String cmdAuthLogin = "auth login";
	const String cmdMailFrom = "MAIL From:";
	const String cmdRcptTo = "RCPT To:";
	const String RcptMail = sensor.email; //"mdmanutenzioni@gmail.com"; //"alefox03@gmail.com"
	const String cmdData = "DATA";
	const String clientEmail = sensor.name+ emailDomain;
	const String baseUser = "alessio.mudu0@gmail.com";
	char server[] = "mail.smtp2go.com";
	int port = 2525; // You can also try using Port Number 25, 8025 or 587.

	if (!clientSMTP2GO.connected()) {
		if (clientSMTP2GO.connect(server, port)) {
			Serial.println(F("connected to mail.smtp2go.com"));
			if (!eRcv()) return;
		}
		else {
			Serial.println(F("connection failed"));
			return;
		}
	}


	clientSMTP2GO.println(cmdEHLO + " freddosicuro.it");
	if (!eRcv()) return;

	clientSMTP2GO.println(cmdAuthLogin);
	if (!eRcv()) return;

	String encodedClientID = _base64.encode(sensor.name+ emailDomain, false);
	clientSMTP2GO.println(encodedClientID); 
	if (!eRcv()) return;

	String encodedpsw = _base64.encode(commonPsw, false);
	clientSMTP2GO.println(encodedpsw); 
	if (!eRcv()) return;

	clientSMTP2GO.println(cmdMailFrom + " <" + baseUser + ">");
	if (!eRcv()) return;

	clientSMTP2GO.println(cmdRcptTo + " <" + RcptMail + ">");
	if (!eRcv()) return;

	clientSMTP2GO.println(cmdData);
	if (!eRcv()) return;

	Config *lConfig = sensor.config;

	clientSMTP2GO.println("Subject: ALLARME TEMPERATURE: "+ sensor.name);
	clientSMTP2GO.println("From: "+ sensor.name + " <" + clientEmail+ ">");
	clientSMTP2GO.println("To: " + lConfig->clientID +" <" + sensor.email +">");
	clientSMTP2GO.println("Attenzione, il sensore " + sensor.name + " (ID:" + String(sensor.id) + ") ha rilevato una temperatura fuori intervallo per due letture consecutive.");
	clientSMTP2GO.println();
	clientSMTP2GO.println("Lettura 1: " + String(sensor._prevTemp));
	clientSMTP2GO.println();
	clientSMTP2GO.println("Lettura 2: " + String(sensor._lastTemp));
	clientSMTP2GO.println();
	clientSMTP2GO.println("L'intervallo di temperatura impostato per questo sensore: " +
		String(sensor.tempMIN) + " °C a " + String(sensor.tempMAX) + "°C.");
	clientSMTP2GO.println();
	clientSMTP2GO.println();
	clientSMTP2GO.println("Puoi controllare lo storico su http://temp.giacob.be/list");
	clientSMTP2GO.println();
	clientSMTP2GO.println();
	clientSMTP2GO.println("Distinti saluti da FreddoSicuro.it! ");
	clientSMTP2GO.println(".");
	if (!eRcv()) return;

	clientSMTP2GO.println("QUIT");
	if (!eRcv()) return;

	clientSMTP2GO.stop();

}












