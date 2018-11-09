//  Arduino <> Wemos D1 Mini pins: https://github.com/esp8266/Arduino/blob/master/variants/d1_mini/pins_arduino.h

// Needed for DS18B20 sensor
#include <OneWire.h>
#include <DallasTemperature.h>

// Needed for the OLED display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSans9pt7b.h>

#define ONE_WIRE_BUS 15      // Arduino 15 = ESP8266 D8
#define SENSOR_RESOLUTION 9  // 9 bits = 94ms max conversion time
#define SENSOR_INDEX 0
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress sensorDeviceAddress;

// Temperature variables
float tempC;          //
float tempAvg;        //
float tempHigh = 100; // Turn the fan on at or above this temp
float tempLow = 95;   // Turn the fan off at or below this temp
bool  fanState;

// This will be the gate pin for the MOSFET
#define RelayPin 4  // Arduino D4 = ESP8266 Pin D2

// OLED Display setup
#define OLED_SDA 14 // Arduino 14 = ESP8266 Pin D5
#define OLED_SCL 12 // Arduino 12 = ESP8266 Pin D6
#define OLED_RESET 16
#define OLED_I2C 0x3C
Adafruit_SSD1306 display(OLED_RESET);
#if (SSD1306_LCDHEIGHT != 64)
#error("Height incorrect, please fix Adafruit_SSD1306.h");
// You will need to modify the Adafruit_SSD1306.h file
// Step 1: uncomment this line: #define SSD1306_128_64
// Step 2: add a comment to this line: #define SSD1306_128_32
#endif

// Define the info needed for the temperature averaging
const int numReadings = 8;     // the number of readings to average
float readings[numReadings];   // the readings from the sensor
int readIndex = 0;             // the index of the current reading
float total = 0;               // the running total
float average = 0;             // the average

// Keep track of timing
uint32_t now = 0;                // All timers reference the now variable

// Loop timer
uint32_t prevLoopMillis;
uint32_t numLoops = 0;
uint32_t currLoops = 0;

// Serial timer
const int serialInterval = 500;
uint32_t prevSerialMillis = now;

// Avg timer
const int avgInterval = 100;
uint32_t prevAvgMillis = now;

// OLED timer
const int OLEDInterval = 250;
uint32_t prevOLEDMillis = now;


void setup() {
  Serial.begin(115200); //Start a serial session

  pinMode(RelayPin, OUTPUT);    // Set the
  digitalWrite(RelayPin, LOW);

  // initialize the DS18B20 temperature sensor
  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, SENSOR_RESOLUTION);

  // initialize all the readings to 0:
  for (int thisReading = 0; thisReading < numReadings; thisReading++)
  {
    readings[thisReading] = 0;
  }

  // Enable I2C communication
  Wire.setClock(400000L);          // ESP8266 Only
  Wire.begin(OLED_SDA, OLED_SCL);  // ESP8266 Only

  // Setup the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C);  // initialize with the I2C addr
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.display();
} // End setup()


float readSensor()
{
  sensors.requestTemperatures();
  tempC = sensors.getTempCByIndex(0);
  return tempC;
}


void avgTemps()
{
  if (now - prevAvgMillis > avgInterval)
  {
    prevAvgMillis = now;

    // subtract the last reading:
    total = total - readings[readIndex];
    // Read the temps from the thermocouple
    readings[readIndex] = readSensor();
    // add the reading to the total:
    total = total + readings[readIndex];
    // advance to the next position in the array:
    readIndex = readIndex + 1;
    // if we're at the end of the array...
    if (readIndex >= numReadings)
    {
      // ...wrap around to the beginning:
      readIndex = 0;
    }

    tempAvg = total / numReadings;
  }
}


void controlRelay(void)
{
  if ( tempAvg >= tempHigh )
  {
    digitalWrite(RelayPin, HIGH);
    fanState = 1;
  }
  else if ( tempAvg <= tempLow )
  {
    digitalWrite(RelayPin, LOW);
    fanState = 0;
  }
}


void displaySerial()
{
  if (now - prevSerialMillis > serialInterval)
  {
    prevSerialMillis = now;

    Serial.print("Raw: ");
    Serial.print(tempC);
    Serial.print(", Avg: ");
    Serial.print(tempAvg);
    Serial.print(", Fan State: ");
    Serial.print(fanState);
    Serial.print(", Loops: ");
    Serial.print(currLoops);
    Serial.print("\n");
  }

}


void displayOLED()
{
  if (now - prevOLEDMillis > OLEDInterval)
  {
    prevOLEDMillis = now;

    // have to wipe the buffer before writing anything new
    display.clearDisplay();

    // TOP HALF = Avg + Average Temp
    display.setFont(&FreeSans9pt7b);
    display.setCursor(0, 22);
    display.print("Avg");

    display.setFont(&FreeSerifBold18pt7b);
    display.setCursor(44, 26);
    if ( tempAvg >= 100 )
      display.print(tempAvg, 1);
    else
      display.print(tempAvg, 0);

    // BOTTOM HALF = Raw + tempC
    display.setFont(&FreeSans9pt7b);
    display.setCursor(0, 56);
    display.print("Raw");

    display.setFont(&FreeSerifBold18pt7b);
    display.setCursor(44, 60);
    if ( tempC >= 100 )
      display.print(tempC, 1);
    else
      display.print(tempC, 0);

    display.display();
  }
}


void trackloop() {
  if ( now - prevLoopMillis >= 1000) {
    currLoops = numLoops;
    numLoops = 0;
    prevLoopMillis = now;
  }
  numLoops++;
}


void loop() {
  now = millis();
  trackloop();
  avgTemps();
  controlRelay();
  displaySerial();
  displayOLED();
}
