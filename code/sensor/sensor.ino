#include <SDS011.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

//needed for Wifi setup library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

//for LED status
#include <Ticker.h>
Ticker ticker;
//Define input PIN and type for DHT11 Sensor
#define DHTPIN D3     // what digital pin we're connected to
#define DHTTYPE DHT11   // DHT 11

//Define PIN for SDS011
// SoftSerial RX PIN is D5 and goes to SDS TX
// SoftSerial TX PIN is D6 and goes to SDS RX
#define SDS_RX D5
#define SDS_TX D6

//define threshold for action
#define DUST_SAFE_THRESHOLD 15
#define DUST_DANGER_THRESHOLD 30
#define HEAT_INDEX_COLD_THRESHOLD 22
#define HEAT_INDEX_COOL_THRESHOLD 28
#define HEAT_INDEX_HOT_THRESHOLD 38

//define Wifi LED
#define WIFI_LED_PIN D6
#define BUILTIN_LED D4
//define Wifi MSG
#define WIFI_ERROR "Can't connect"
#define WIFI_SUCCESS "WIFI ok!"

//LCD
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x20 for a 16 chars and 2 line display

//DHT11 Sensor
DHT dht(DHTPIN, DHTTYPE);

//Nova SDS011 Sensor
SDS011 sds;

//pm10 and pm2.5 dust data
float p10, p25;

//error handler for SDS011
int error;

void setup()
{
  //Output to Serial Console
  Serial.begin(9600);

  //Start SDS011
  sds.begin(SDS_RX, SDS_TX);

  //Start LCD
  lcd.init();                      // initialize the lcd
  // Print a message to the LCD.
  // Set backlight is on
  lcd.backlight();

  // Set position
  lcd.setCursor(0, 0);
  // Set start msg
  lcd.print("Loading...");

  //Start DHT Sensor
  dht.begin();                    // initialize DHT sensor

  startWifi();
}

void loop()
{
  delay(3000);
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

  //Print to LCD
  String tempBuffer;
  tempBuffer += F("Temp: ");
  tempBuffer += String(t, 1);
  tempBuffer += F("*C");
  printFirstLine(tempBuffer);

  String humidityBuffer;
  humidityBuffer += F("Humid: ");
  humidityBuffer += String(h, 1);
  humidityBuffer += F("%");
  printSecondLine(humidityBuffer);

  //display Heat Index on LCD
  delay(3000);
  lcd.clear();
  String tempIndexBuffer;
  tempIndexBuffer += F("HeatIndex:");
  tempIndexBuffer += String(hic, 1);
  tempIndexBuffer += F("*C");
  printFirstLine(tempIndexBuffer);
  //Create suitable action and display on LCD
  if (hic < HEAT_INDEX_COLD_THRESHOLD)
  {
    printSecondLine("Keep warm!");
  }
  else if (hic < HEAT_INDEX_COOL_THRESHOLD)
  {
    printSecondLine("It's okay!");
  }
  else if (hic < HEAT_INDEX_HOT_THRESHOLD)
  {
    printSecondLine("Be careful");
  }
  else {
    printSecondLine("DANGER!");
  }


  //  Comment due to SDS011 error
  delay(3000);
  lcd.clear();
  //Read data from SDS011
  error = sds.read(&p25, &p10);
  if (!error) {
    String pm25Buffer;
    pm25Buffer += F("PM2.5:");
    pm25Buffer += String(normalizePM25(p25 / 10, h), 1);
    printFirstLine(pm25Buffer);

    String pm10Buffer;
    pm10Buffer += F("PM10:");
    pm10Buffer += String(normalizePM10(p10 / 10, h), 1);
    printFirstLine(pm10Buffer);

  } else {
    printFirstLine("SDS011 error");
  }
  //Demo data
  delay(1000);
  lcd.clear();
  int randomPm25 = random(10, 40);
  String pm25DemoBuffer;
  pm25DemoBuffer += F("PM2.5:");
  pm25DemoBuffer += String(randomPm25);
  printFirstLine(pm25DemoBuffer);
  if (randomPm25 < DUST_SAFE_THRESHOLD)
  {
    printSecondLine("Safe");
  }
  else if (randomPm25 < DUST_DANGER_THRESHOLD)
  {
    printSecondLine("Be careful!");
  }
  else {
    printSecondLine("DANGER!");
  }
}

//Function for print first line on LCD
void printFirstLine(String text)
{
  lcd.setCursor(0, 0);
  lcd.print(text);
}

//Function for print second line on LCD
void printSecondLine(String text)
{
  lcd.setCursor(0, 1);
  lcd.print(text);
}
////////////////WIFI////////////////////////////////
//Tick of LED
void tick()
{
  //toggle state
  int state = digitalRead(WIFI_LED_PIN);  // get the current state of GPIO1 pin
  digitalWrite(WIFI_LED_PIN, !state);     // set pin to the opposite state
}

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager) {
  lcd.clear();
  printFirstLine("Config mode");
  delay(1000);
  lcd.clear();
  printFirstLine(String(WiFi.softAPIP()));
  //if you used auto generated SSID, print it
  printSecondLine(myWiFiManager->getConfigPortalSSID());
  //entered config mode, make led toggle faster
  ticker.attach(0.2, tick);
}

//start Wifi
void startWifi()
{
  delay(1000);
  //Start Wifi Mananger, set D4 led pin as output
  pinMode(WIFI_LED_PIN, OUTPUT);
  // start ticker with 0.5 because esp0-12 will start in AP mode and try to connect
  ticker.attach(0.5, tick);
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;
  //reset settings - for testing
  //wifiManager.resetSettings();

  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  lcd.clear();
  if (!wifiManager.autoConnect()) {
    printFirstLine(WIFI_ERROR);
    delay(1000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
  }

  //if you get here you have connected to the WiFi
  printFirstLine(WIFI_SUCCESS);
  ticker.detach();
  //keep LED on
  digitalWrite(WIFI_LED_PIN, HIGH);
}

////////////////SDS011 correction/////////////////////////////////////
//Correction algorithm thanks to help of Zbyszek Kiliański (Krakow Zdroj)
float normalizePM25(float pm25, float humidity) {
  return pm25 / (1.0 + 0.48756 * pow((humidity / 100.0), 8.60068));
}

float normalizePM10(float pm10, float humidity) {
  return pm10 / (1.0 + 0.81559 * pow((humidity / 100.0), 5.83411));
}

float calculatePolutionPM25(float pm25) {
  return pm25 * 100 / 25;
}

float calculatePolutionPM10(float pm10) {
  return pm10 * 100 / 50;
}
