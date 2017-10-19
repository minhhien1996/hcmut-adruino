#include <SDS011.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include <SoftwareSerial.h>

#define DHTPIN D3     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

// SoftSerial RX PIN is D1 and goes to SDS TX
// SoftSerial TX PIN is D2 and goes to SDS RX
#define SDS__RX D6
#define SDS_TX D7

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display
DHT dht(DHTPIN, DHTTYPE);


struct {
  bool valid;
  float pm10; // 10
  float pm25; // 2.5
} sds011_result = {};

void setup()
{
  Serial.begin(115200);     // Output to USB/TTL
  serialSDS.begin(9600);
  
  lcd.init();                      // initialize the lcd 
  dht.begin();                    // initialize DHT sensor
  // Print a message to the LCD.
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("Loading...");

}

void loop()
{
  delay(500);
  lcd.clear();
  // Reading temperature or humidity takes about 250 milliseconds!
  // Sensor readings may also be up to 2 seconds 'old' (its a very slow sensor)
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  
  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f)) {
    printFirstLine("Sensor Error");
    return;
  }
  
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  String tempBuffer;
  tempBuffer+= F("Temp: ");
  tempBuffer+= String(t, 1);
  tempBuffer+= F("*C");
  printFirstLine(tempBuffer);

  String humidityBuffer;
  humidityBuffer+= F("Humid: ");
  humidityBuffer+= String(h, 1);
  humidityBuffer+= F("%");
  printSecondLine(humidityBuffer);
  delay(1000);
  readSDS();
}

void printFirstLine(String text)
{
  lcd.setCursor(0, 0);
  lcd.print(text);
}

void printSecondLine(String text)
{
  lcd.setCursor(0, 1);
  lcd.print(text);
}


