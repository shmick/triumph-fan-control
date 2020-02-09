// Needed for DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS D7
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;

// Temperature variables
float TempC;
float TempHigh = 100; // Turn the fan on at or above this temp
float TempLow = 95;   // Turn the fan off at or below this temp
bool FanState;

// This will be the gate pin of the BS170 MOSFET
#define RelayPin D2

// Needed for IotWebConf https://github.com/prampec/IotWebConf
#include <IotWebConf.h>

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

void getTemps(void)
{
  sensors.requestTemperatures();
  TempC = sensors.getTempCByIndex(0);
}


void controlRelay(void)
{
  if ( TempC >= TempHigh )
  {
    digitalWrite(RelayPin, HIGH);
    FanState = 1;
  }
  else if ( TempC <= TempLow )
  {
    digitalWrite(RelayPin, LOW);
    FanState = 0;
  }
}


void handleRoot()
{
  // -- Let IotWebConf test and handle captive portal requests.
  if (iotWebConf.handleCaptivePortal())
  {
    // -- Captive portal request were already served.
    return;
  }
  String s = "<!DOCTYPE html><html lang=\"en\"><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1, user-scalable=no\"/>";
  s += "<title>IotWebConf 04 Update Server</title></head><body>Hello world!";
  s += "Go to <a href='config'>configure page</a> to change values.";
  s += "</body></html>\n";

  server.send(200, "text/html", s);
}


void handleStats() {
  String message = "<head> <meta http-equiv=\"refresh\" content=\"2\"> </head>\n";
  message +=  "<h1>\n";
  message += "Temp: " + String(TempC) + "<br>\n";
  message += "Fan: " + String(FanState);
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
  iotWebConf.init();
  
  server.on("/", handleRoot);
  server.on("/config", [] { iotWebConf.handleConfig(); });
  server.on("/stats", handleStats);
  server.onNotFound([]() { iotWebConf.handleNotFound(); });
}


void loop() {
  getTemps();
  controlRelay();
  esp8266Tasks();
  now = millis();
}
