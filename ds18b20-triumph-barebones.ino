//  Arduino <> Wemos D1 Mini pins: https://github.com/esp8266/Arduino/blob/master/variants/d1_mini/pins_arduino.h

// Needed for DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 15 // Arduino 15 = ESP8266 D8
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;

// Temperature variables
float TempC;          //
float TempHigh = 100; // Turn the fan on at or above this temp
float TempLow = 95;   // Turn the fan off at or below this temp
bool FanState;

// This will be the gate pin on our BS170 MOSFET
#define RelayPin 4 // Arduino D4 = ESP8266 Pin D2

void setup() {
  // Set the Relay to output mode and ensure the relay is off
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);

  // initialize the DS18B20 temperature sensor
  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, 9);
}


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


void displaySerial()
{
  Serial.print("Temp: ");
  Serial.println(TempC);
  Serial.print("Fan State: ");
  Serial.println(FanState);
}


void loop() {
  getTemps();
  controlRelay();
  displaySerial();
}
