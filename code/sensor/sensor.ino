#include <SDS011.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

#define DHTPIN D3     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

// SoftSerial RX PIN is D1 and goes to SDS TX
// SoftSerial TX PIN is D2 and goes to SDS RX
#define SDS_RX D5
#define SDS_TX D6

LiquidCrystal_I2C lcd(0x27,16,2);  // set the LCD address to 0x20 for a 16 chars and 2 line display
DHT dht(DHTPIN, DHTTYPE);
SDS011 sds;
float p10,p25;
int error;

struct Air {
  float pm25;
  float pm10;
  float humidity;
  float temperature;
};

void setup()
{
  Serial.begin(9600);     // Output to USB/TTL
  sds.begin(SDS_RX,SDS_TX);
  
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
   error = sds.read(&p25,&p10);
  if (!error) {  
    Serial.println(String(normalizePM25(p25/10, h), 1));
    Serial.println(String(normalizePM10(p10/10, h), 1));
  } else {
    Serial.println("Error reading SDS011");
  }
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

//Correction algorythm thanks to help of Zbyszek Kiliański (Krakow Zdroj)
float normalizePM25(float pm25, float humidity){
  return pm25/(1.0+0.48756*pow((humidity/100.0), 8.60068));
}

float normalizePM10(float pm10, float humidity){
  return pm10/(1.0+0.81559*pow((humidity/100.0), 5.83411));
}

float calculatePolutionPM25(float pm25){
  return pm25*100/25;
}

float calculatePolutionPM10(float pm10){
  return pm10*100/50;
}