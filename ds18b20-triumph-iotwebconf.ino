// Needed for DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D7
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;

// Temperature variables
float TempC;
int TempHigh = 100; // Turn the fan on at or above this temp
int TempLow = 95;   // Turn the fan off at or below this temp
bool FanState;

int ValueFanOn = 0;
int ValueFanOff = 0;

// This will be the gate pin of the BS170 MOSFET
#define RelayPin D2

// Needed for IotWebConf https://github.com/prampec/IotWebConf
#include <IotWebConf.h>

#define NUMBER_LEN 32
char intParamValueFanOn[NUMBER_LEN];
char intParamValueFanOff[NUMBER_LEN];

// Server tasks interval
uint32_t now = 0; //This variable is used to keep track of time
const int serverInterval = 50;
uint32_t currentServerMillis;
uint32_t previousServerMillis = now;

// -- Initial name of the Thing. Used e.g. as SSID of the own Access Point.
const char thingName[] = "fan";
// -- Initial password to connect to the Thing, when it creates an own Access Point.
const char wifiInitialApPassword[] = "11111111";

DNSServer dnsServer;
WebServer server(80);
HTTPUpdateServer httpUpdater;
IotWebConf iotWebConf(thingName, &dnsServer, &server, wifiInitialApPassword);
IotWebConfSeparator separator1 = IotWebConfSeparator();
IotWebConfParameter intParamFanOn = IotWebConfParameter("Fan On", "intParamValueFanOn", intParamValueFanOn, NUMBER_LEN, "number", "1..125", NULL, "min='1' max='125' step='1'");
IotWebConfParameter intParamFanOff = IotWebConfParameter("Fan Off", "intParamValueFanOff", intParamValueFanOff, NUMBER_LEN, "number", "1..125", NULL, "min='1' max='125' step='1'");

// IotWebConfParameter timer
const int ParamInterval = 2000;
uint32_t previousParamMillis = now;

void getTemps(void)
{
  sensors.requestTemperatures();
  TempC = sensors.getTempCByIndex(0);
}


void getParams(void)
{
  uint32_t currentParamMillis = now;

  if (currentParamMillis - previousParamMillis > ParamInterval) {
    previousParamMillis = currentParamMillis;

    if ( atoi(intParamValueFanOn) == 0 ) {
      ValueFanOn = TempHigh;
    } else {
      ValueFanOn = atoi(intParamValueFanOn);
    }

    if ( atoi(intParamValueFanOff) == 0 ) {
      ValueFanOff = TempLow;
    } else {
      ValueFanOff = atoi(intParamValueFanOff);
    }
  }
}


void controlRelay(void)
{
  getParams();
  // Only turn on the fan if ValueFanOn is set to a number greater than 1
  // Only turn on the fan if ValueFanOn is greater than ValueFanOff
  if ( TempC >= ValueFanOn && ( ValueFanOn > 1 || ValueFanOn > ValueFanOff ))
  {
    digitalWrite(RelayPin, HIGH);
    FanState = 1;
  }
  // If ValueFanOff is not less than ValueFanOn, don't turn off the fan 
  else if ( TempC <= ValueFanOff && ValueFanOff < ValueFanOn )
  {
    digitalWrite(RelayPin, LOW);
    FanState = 0;
  }
}


void handleRoot()
{
  /*
    // -- Let IotWebConf test and handle captive portal requests.
    if (iotWebConf.handleCaptivePortal())
    {
      // -- Captive portal request were already served.
      return;
    }
  */
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>Fan Controller</title></head><body>";
  s += "<ul>";
  s += "<li>Fan On param value: ";
  s += atoi(intParamValueFanOn);
  s += "<li>Fan Off param value: ";
  s += atoi(intParamValueFanOff);
  s += "</ul>";
  s += "Go to <a href='stats'>stats page</a> to see realtime stats.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}


void handleStats() {
  String message = "<head> <meta http-equiv=\"refresh\" content=\"2\"> </head>\n";
  message +=  "<h1>\n";
  message += "Current Temp: " + String(TempC) + "<br>\n";
  message += "Fan State: " + String(FanState) + "<br>\n";
  message += "Param Fan On: " + String(atoi(intParamValueFanOn)) + "<br>\n";
  message += "Param Fan Off: " + String(atoi(intParamValueFanOff)) + "<br>\n";
  message += "SetValue Fan On: " + String(ValueFanOn) + "<br>\n";
  message += "SetValue Fan Off: " + String(ValueFanOff);
  server.send(200, "text/html", message);
}


void esp8266Tasks() {
  currentServerMillis = now;
  if (currentServerMillis - previousServerMillis > serverInterval) {
    iotWebConf.doLoop();
    previousServerMillis = currentServerMillis;
  }
}



void setup() {

  // Set the Relay to output mode and ensure the relay is off
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);

  // initialize the DS18B20 temperature sensor
  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, 9);

  iotWebConf.setupUpdateServer(&httpUpdater);

  iotWebConf.addParameter(&separator1);
  iotWebConf.addParameter(&intParamFanOn);
  iotWebConf.addParameter(&intParamFanOff);

  iotWebConf.init();

  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.on("/stats", handleStats);
  server.onNotFound([]() {
    iotWebConf.handleNotFound();
  });
}


void loop() {
  getTemps();
  controlRelay();
  esp8266Tasks();
  now = millis();
}
