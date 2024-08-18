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

// NTP Client to get time (Brussels timezone is UTC + 2 hours during DST, adjust as needed)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Brussels is UTC+1, adjust 3600 for standard time, 7200 for DST

// Display intervals (in milliseconds)
const unsigned long displayBitcoinPriceDuration = 21000; // 21 seconds
const unsigned long displayTimeDuration = 30000; // 30 seconds

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
    // Alternate between displaying Bitcoin price and time
    displayBitcoinPrice(apiEndpointUSD, "USD");
    delay(displayBitcoinPriceDuration); // Show Bitcoin price for the configured duration

    displayTime();
    delay(displayTimeDuration); // Show time for the configured duration
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

    // Convert price string to individual characters
    char priceChars[6]; // Limit to 5 characters
    priceFirst5.toCharArray(priceChars, 6);

    // Display the price on MAX7219 display
    mx.clear();
    int length = strlen(priceChars);

    // Start displaying characters from 5 pixels to the front
    int x = 5;

    // Display characters from the array in reverse order (mirrored)
    for (int i = length - 1; i >= 0; i--) {
      char c = priceChars[i];
      mx.setChar(x, c);
      x += 6; // Move to the next character position (assuming 6 pixels per character)
    }
    mx.update(); // Update the display
  } else {
    Serial.println("Error fetching Bitcoin price for " + String(currency));
  }

  http.end();
}

void displayTime() {
  timeClient.update();
  unsigned long rawTime = timeClient.getEpochTime();
  rawTime += 3600; // Adjust for the timezone, change this offset according to DST

  unsigned long hours = (rawTime  % 86400L) / 3600;
  unsigned long minutes = (rawTime % 3600) / 60;

  // Format time as HH:MM
  char timeChars[6];
  sprintf(timeChars, "%02lu:%02lu", hours, minutes);

  // Display the time on MAX7219 display
  mx.clear();
  int x = 5; // Start displaying characters from 5 pixels to the front

  // Display characters from the array
  for (int i = 0; i < 5; i++) {
    char c = timeChars[i];
    mx.setChar(x, c);
    x += 6; // Move to the next character position (assuming 6 pixels per character)
  }
  mx.update(); // Update the display
}
