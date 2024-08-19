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

// NTP Client to get time (Brussels timezone is UTC + 1 hour, adjust as needed)
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 3600, 60000); // Brussels is UTC+1

// Display intervals (in milliseconds)
const unsigned long displayBitcoinPriceDuration = 21000; // 21 seconds
const unsigned long displayTimeDuration = 30000; // 30 seconds
const unsigned long priceUpdateInterval = 60000; // Update price every minute

unsigned long lastSwitchTime = 0;
unsigned long lastPriceUpdateTime = 0;
bool showingPrice = false;
String currentPrice = "";

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

  // Initial price fetch
  updateBitcoinPrice();
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    unsigned long currentTime = millis();
    
    // Update price every minute
    if (currentTime - lastPriceUpdateTime >= priceUpdateInterval) {
      lastPriceUpdateTime = currentTime;
      updateBitcoinPrice();
    }

    // Toggle between showing price and time
    if (showingPrice) {
      if (currentTime - lastSwitchTime >= displayBitcoinPriceDuration) {
        lastSwitchTime = currentTime;
        showingPrice = false;
        mx.clear();
      } else {
        displayBitcoinPrice();
      }
    } else {
      if (currentTime - lastSwitchTime >= displayTimeDuration) {
        lastSwitchTime = currentTime;
        showingPrice = true;
        mx.clear();
      } else {
        displayTime();
      }
    }
  }

  delay(1000); // Short delay to avoid rapid looping
}

void updateBitcoinPrice() {
  HTTPClient http;
  http.begin(apiEndpointUSD);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, payload);
    currentPrice = doc["bpi"]["USD"]["rate"].as<String>();

    // Remove commas from the price string
    currentPrice.replace(",", "");

    // Get only the first 5 characters of the price
    currentPrice = currentPrice.substring(0, 5);
  } else {
    Serial.println("Error fetching Bitcoin price");
    currentPrice = "ERROR";
  }

  http.end();
}

void displayBitcoinPrice() {
  // Display the price on MAX7219 display
  mx.clear();
  int x = 5; // Start at position 5 to match the desired display position

  for (int i = 0; i < currentPrice.length(); i++) {
    char c = currentPrice[i];
    mx.setChar(x, c);
    x += 6; // Move to the next character position (assuming 6 pixels per character)
  }
  mx.update(); // Update the display
}

void displayTime() {
  timeClient.update();
  unsigned long rawTime = timeClient.getEpochTime();
  rawTime += 3600; // Adjust for the timezone, change this offset according to DST

  unsigned long hours = (rawTime % 86400L) / 3600;
  unsigned long minutes = (rawTime % 3600) / 60;

  // Format time as HH:MM
  char timeChars[6];
  sprintf(timeChars, "%02lu:%02lu", hours, minutes);

  // Display the time on MAX7219 display
  mx.clear();
  int x = 5; // Start at position 5 to match the price's display position

  for (int i = 0; i < strlen(timeChars); i++) {
    char c = timeChars[i];
    mx.setChar(x, c);
    x += 6; // Move to the next character position (5 pixels + 1 space)
  }
  mx.update(); // Update the display
}
