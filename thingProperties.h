#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

const char DEVICE_LOGIN_NAME[]  = "device-name";

const char SSID[]               = "network-name";    // Network SSID (name)
const char PASS[]               = "network-password";    // Network password (use for WPA, or use as key for WEP)
const char DEVICE_KEY[]  = "secret";    // Secret device password


float switchState;
int motorCounter;

void initProperties(){

  ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
  ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
  ArduinoCloud.addProperty(switchState, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(motorCounter, READ, ON_CHANGE, NULL);
  ArduinoCloud.setThingId("thing-id");

}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);
