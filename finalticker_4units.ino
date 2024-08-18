#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#include <TimeLib.h>
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

// API endpoints
const char* apiEndpointUSD = "https://api.coindesk.com/v1/bpi/currentprice/USD.json";
const char* apiEndpointBlockHeight = "https://mempool.space/api/blocks/tip/height";

// NTP client to get the time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

// Timezone offset in seconds (default Brussels, UTC+1)
long timezoneOffset = 3600;

int lastBlockHeight = 0;

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

  // Initialize the time client with the timezone offset
  timeClient.begin();
  timeClient.setTimeOffset(timezoneOffset);
}

void loop() {
  static unsigned long lastUpdate = 0;
  timeClient.update();

  // Display time continuously
  displayTime();

  // Every minute, show the Bitcoin price for a few seconds
  if (millis() - lastUpdate >= 60000) {
    if (WiFi.status() == WL_CONNECTED) {
      displayBitcoinPrice(apiEndpointUSD, "USD");
    }
    lastUpdate = millis();
  }

  // Check for a new block
  if (WiFi.status() == WL_CONNECTED) {
    int currentBlockHeight = getLatestBlockHeight();
    if (currentBlockHeight > lastBlockHeight) {
      displayNewBlockHeight(currentBlockHeight);
      lastBlockHeight = currentBlockHeight;
    }
  }

  delay(1000); // Update the time display every second
}

void displayTime() {
  mx.clear();
  
  // Get the current time
  int hours = timeClient.getHours();
  int minutes = timeClient.getMinutes();

  // Format the time as HH:MM
  char timeString[6];
  sprintf(timeString, "%02d:%02d", hours, minutes);

  // Display the time on MAX7219
  mx.print(timeString);
  mx.update();
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
    delay(5000); // Display time for 5 seconds
  } else {
    Serial.println("Error fetching Bitcoin price for " + String(currency));
  }

  http.end();
}

int getLatestBlockHeight() {
  HTTPClient http;
  http.begin(apiEndpointBlockHeight);
  int httpResponseCode = http.GET();

  int blockHeight = -1;

  if (httpResponseCode > 0) {
    String payload = http.getString();
    blockHeight = payload.toInt();
  } else {
    Serial.println("Error fetching latest block height");
  }

  http.end();
  return blockHeight;
}

void displayNewBlockHeight(int blockHeight) {
  // Convert block height to string
  char blockHeightString[10];
  sprintf(blockHeightString, "%d", blockHeight);

  for (int i = 0; i < 3; i++) {
    mx.clear();
    mx.print(blockHeightString);
    mx.update();
    delay(500); // On for 500ms
    mx.clear();
    delay(500); // Off for 500ms
  }

  // Display the block height for a few seconds after flicker
  mx.clear();
  mx.print(blockHeightString);
  mx.update();
  delay(5000); // Display for 5 seconds
}
