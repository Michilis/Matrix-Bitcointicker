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

// API endpoints
const char* apiEndpointUSD = "https://api.coindesk.com/v1/bpi/currentprice/USD.json";
const char* apiBlockHeight = "https://blockchain.info/q/getblockcount";

// NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 60000); // Update time every minute

// Variable to store the last fetched block height
long lastBlockHeight = 0;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
  
  // Initialize the MAX7219 display
  SPI.begin(CLK_PIN, MISO, DATA_PIN, CS_PIN);
  SPI.setFrequency(1000000); // Set SPI baud rate to 1 MHz
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
    
    // Check for new block mined
    checkNewBlockMined(apiBlockHeight);
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
    for (int i = 0; i < priceFirst5.length(); i++) {
      mx.setChar(i * 8, priceFirst5[i]);
    }
    mx.update(); // Update the display
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
  for (int i = 0; i < timeToDisplay.length(); i++) {
    mx.setChar(i * 8, timeToDisplay[i]);
  }
  mx.update(); // Update the display
}

void checkNewBlockMined(const char* apiEndpoint) {
  HTTPClient http;
  http.begin(apiEndpoint);
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    long currentBlockHeight = payload.toInt();

    if (currentBlockHeight > lastBlockHeight) {
      lastBlockHeight = currentBlockHeight;
      
      // Get the last 5 digits of the block height
      String blockHeightStr = String(currentBlockHeight);
      String blockHeightLast5 = blockHeightStr.substring(blockHeightStr.length() - 5);

      // Flash the block height 3 times
      for (int i = 0; i < 3; i++) {
        mx.clear();
        for (int j = 0; j < blockHeightLast5.length(); j++) {
          mx.setChar(j * 8, blockHeightLast5[j]);
        }
        mx.update();
        delay(500); // Show for 500ms
        mx.clear();
        delay(500); // Off for 500ms
      }
      
      // Show the block height for an additional 5 seconds
      mx.clear();
      for (int i = 0; i < blockHeightLast5.length(); i++) {
        mx.setChar(i * 8, blockHeightLast5[i]);
      }
      mx.update();
      delay(5000); // Display time for 5 seconds
    }
  } else {
    Serial.println("Error fetching block height");
  }

  http.end();
}
