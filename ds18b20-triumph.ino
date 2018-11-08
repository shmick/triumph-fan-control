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


// Keep track of timing
uint32_t now = 0; //This variable is used to keep track of time
const int LoopInterval = 100;
uint32_t currentMillis;
uint32_t previousMillis = now;

void setup() {
  // Set the Relay to output mode and ensure the relay is off
  pinMode(RelayPin, OUTPUT);
  digitalWrite(RelayPin, LOW);

  // initialize the DS18B20 temperature sensor
  sensors.begin();
  sensors.getAddress(sensorDeviceAddress, 0);
  sensors.setResolution(sensorDeviceAddress, 9);

  // Enable I2C communication
  Wire.setClock(400000L); // ESP8266 Only
  Wire.begin(OLED_SDA, OLED_SCL);

  // Setup the OLED display
  display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C);  // initialize with the I2C addr 0x3C (for the 128x32)
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.display();

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


void displayOLED()
{
  // put something on the OLED
}


void loop() {
  // No need to run this more than every 100ms
  now = millis();
  currentMillis = now;
  if (currentMillis - previousMillis > LoopInterval) {
    getTemps();
    controlRelay();
    displaySerial();
    //    displayOLED();
    previousMillis = currentMillis;
  }
}
