/*#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[] = "934160f1-52da-44fd-91d9-5416ee0b3ce5";

const char SSID[] = "";    // Network SSID (name)
const char PASS[] = "";    // Network password (use for WPA, or use as key for WEP)
// IPhone de Fran: holahola
// FD: 44220636
const char DEVICE_KEY[]  = "ko0Z22xy#hlH6NNPSuHjNcYHn";    // Secret device password


float switchState;
int motorCounter;

void initProperties(){

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(switchState, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(motorCounter, READ, ON_CHANGE, NULL);
  ArduinoCloud.setThingId("6ba3b80d-0b63-4ded-9b88-773952418875");

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);*/
