#include <ESP8266WiFi.h>          //ESP8266 Core WiFi Library (you most likely already have this in your sketch)
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

// Needed for DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2 // ESP8266 GPIO2
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;


#define RELAY 0 // relay connected to  GPIO0

ESP8266WebServer server(80);

// Temperature variables
float TempC;          // The variable we will store the temp to
float TempHigh = 100; // Turn the fan on at or above this temp
float TempLow = 95;   // Turn the fan off at or below this temp
bool FanState;        // Tracks the fan state for monitoring

// All timers reference the value of now
uint32_t now = 0; //This variable is used to keep track of time

// Temp read interval
const int TempInterval = 200;
uint32_t currentTempMillis;
uint32_t previousTempMillis = now;

uint32_t prevLoopMillis;
uint32_t numLoops = 0;
uint32_t currLoops = 0;

int runTimeMins;
long runTimeSecs;
uint32_t runTimeStart = now;


// ESP8266WebServer handler
void handleStats() {
  String message = "<head> <meta http-equiv=\"refresh\" content=\"2\"> </head>\n";
  message +=  "<h1>\n";
  message +=  "Loops: " + String(currLoops) + "<br>\n";
  message += "Temp C: " + String(TempC) + "<br>\n";
  message += "FanState: " + String(FanState) + "<br>\n";
  message += "RelayState: " + String(RELAY) + "<br>";
  server.send(200, "text/html", message);
}


void setup(void) {

  pinMode(RELAY, OUTPUT);
  // force the relay default state to off (HIGH)
  digitalWrite(RELAY, HIGH);

  WiFiManager wifiManager;
  wifiManager.setConfigPortalTimeout(90);
  wifiManager.autoConnect();

  // initialize the DS18B20 temperature sensor
  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, 9);

  ArduinoOTA.setHostname("OTARELAY");
  ArduinoOTA.begin();
  MDNS.begin("relay");

  server.on("/stats", HTTP_GET, handleStats); // Output device stats
  server.begin();
}


void getTemps(void)
{
  currentTempMillis = now;
  if (currentTempMillis - previousTempMillis > TempInterval) {
    sensors.requestTemperatures();
    TempC = sensors.getTempCByIndex(0);

    previousTempMillis = currentTempMillis;
  }
}


void controlRelay(void)
{
  if ( TempC >= TempHigh )
  {
    digitalWrite(RELAY, LOW);
    FanState = 1;
  }
  else if ( TempC <= TempLow )
  {
    digitalWrite(RELAY, HIGH);
    FanState = 0;
  }
}


void keepTime(void)
{
  //Keep track of time
  now = millis();
  runTimeSecs = (now - runTimeStart) / 1000;
  runTimeMins = (now - runTimeStart) / 60000;
}


// Track how many loops per second are executed.
void trackloop() {
  if ( now - prevLoopMillis >= 1000) {
    currLoops = numLoops;
    numLoops = 0;
    prevLoopMillis = now;
  }
  numLoops++;
}


void loop(void) {
  keepTime();
  getTemps();
  controlRelay();
  trackloop();
  server.handleClient();
  ArduinoOTA.handle();
}
