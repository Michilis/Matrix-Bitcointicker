#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

// Hardware configuration
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW
#define MAX_DEVICES 4 // Number of 8x8 matrices

// GPIO pins
#define CLK_PIN   18 // VSPI_SCK
#define DATA_PIN  23 // VSPI_MOSI
#define CS_PIN    5  // VSPI_SS

MD_MAX72XX mx = MD_MAX72XX(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

// WiFi credentials
const char* ssid = "keinerhierkeinerda";
const char* password = "mauskatzehund.123";

// API endpoint for fetching Bitcoin prices
const char* apiEndpointUSD = "https://api.coindesk.com/v1/bpi/currentprice/USD.json";

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 7200, 60000); // Brussels time is UTC +2

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Initialize the MAX7219 display
  SPI.begin(CLK_PIN, MISO, DATA_PIN, CS_PIN);
  SPI.setFrequency(1000000); // Set SPI baud rate to 1 MHz (adjust as needed)
  mx.begin();
  mx.control(MD_MAX72XX::INTENSITY, 0); // Set brightness (0-15)
  mx.clear();

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize time client
  timeClient.begin();
  timeClient.update();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Display the current time
    displayTime();
    delay(10000); // Display time for 10 seconds
    
    // Fetch Bitcoin price in USD
    displayBitcoinPrice(apiEndpointUSD, "USD");
    delay(10000); // Display price for 10 seconds
  }

  delay(1000); // Short delay to avoid rapid looping
}

void displayBitcoinPrice(const char* apiEndpoint, const char* currency) {
  HTTPClient http;
  http.begin(apiEndpoint);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    String price = doc["bpi"][currency]["rate"].as<String>();

    // Remove commas from the price string
    price.replace(",", "");

    // Get only the first 5 characters of the price
    String priceFirst5 = price.substring(0, 5);

    // Display the price on MAX7219 display
    mx.clear();
    printText(priceFirst5.c_str());
  } else {
    Serial.println("Error fetching Bitcoin price for " + String(currency));
  }

  http.end();
}

void displayTime() {
  timeClient.update();
  String formattedTime = timeClient.getFormattedTime();
  String timeToDisplay = formattedTime.substring(0, 5); // Display HH:MM

  // Display the time on MAX7219 display
  mx.clear();
  printText(timeToDisplay.c_str());
}

void printText(const char* text) {
  uint8_t len = strlen(text);

  for (uint8_t i = 0; i < len; i++) {
    mx.clear();
    mx.setChar(0, text[i]);
    mx.update();
    delay(1000);
  }
}
