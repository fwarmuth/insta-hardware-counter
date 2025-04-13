#include "counter.h"
#include "matrix_config.h"
#include "color_utils.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// Private counter variables
static unsigned long counter = 0;
static unsigned long prevCounter = 0; // Track previous value for comparison
static unsigned long lastCounterUpdate = 0;
// static const char* API_ENDPOINT = "http://edge.warmuth.xyz:5000/api/instagram/metrics";
static const char* API_ENDPOINT = "http://172.16.10.190:5000/api/instagram/metrics";

// Simplified color - purple-blue
static const uint16_t COUNTER_COLOR = 0x4A1F; // Purple-blue color in RGB565 format

/**
 * @brief Initialize the counter
 */
void initCounter() {
    counter = 0;
    prevCounter = 0;
    lastCounterUpdate = millis();
    
    // Try to get initial value from API
    if(WiFi.status() == WL_CONNECTED) {
        fetchCounterFromAPI();
    }
    displayCounter();
}

/**
 * @brief Fetch follower count from Instagram API
 * @return True if successful
 */
bool fetchCounterFromAPI() {
    bool success = false;
    
    // Check if WiFi is connected
    if(WiFi.status() == WL_CONNECTED) {
        HTTPClient http;
        
        Serial.println("Fetching follower count from API...");
        Serial.print("API Endpoint: ");
        Serial.println(API_ENDPOINT);
        
        // Enable more detailed debugging
        http.setReuse(false);
        
        // Set HTTP request timeout to 30 seconds (30000 ms)
        http.setTimeout(45000);
        
        // Start HTTP connection with more details
        Serial.println("Starting HTTP connection...");
        http.begin(API_ENDPOINT);
        
        // Make GET request
        Serial.println("Sending GET request...");
        int httpResponseCode = http.GET();
        
        // Debug HTTP response code
        Serial.print("HTTP Response Code: ");
        Serial.println(httpResponseCode);
        
        // Interpret error codes
        if(httpResponseCode < 0) {
            switch(httpResponseCode) {
                case HTTPC_ERROR_CONNECTION_REFUSED:
                    Serial.println("Error: Server refused connection");
                    break;
                case HTTPC_ERROR_SEND_HEADER_FAILED:
                    Serial.println("Error: Failed to send headers");
                    break;
                case HTTPC_ERROR_SEND_PAYLOAD_FAILED:
                    Serial.println("Error: Failed to send payload");
                    break;
                case HTTPC_ERROR_NOT_CONNECTED:
                    Serial.println("Error: Not connected to server");
                    break;
                case HTTPC_ERROR_CONNECTION_LOST:
                    Serial.println("Error: Connection lost");
                    break;
                case HTTPC_ERROR_NO_STREAM:
                    Serial.println("Error: No data stream");
                    break;
                case HTTPC_ERROR_NO_HTTP_SERVER:
                    Serial.println("Error: Not an HTTP server");
                    break;
                case HTTPC_ERROR_TOO_LESS_RAM:
                    Serial.println("Error: Not enough RAM");
                    break;
                case HTTPC_ERROR_ENCODING:
                    Serial.println("Error: Transfer encoding error");
                    break;
                case HTTPC_ERROR_STREAM_WRITE:
                    Serial.println("Error: Stream write error");
                    break;
                case HTTPC_ERROR_READ_TIMEOUT:
                    Serial.println("Error: Read timeout");
                    break;
                default:
                    Serial.print("Unknown error: ");
                    Serial.println(httpResponseCode);
                    break;
            }
        }
        
        if(httpResponseCode == 200) {
            // Successful response
            String payload = http.getString();
            Serial.println("API Response: " + payload);
            
            // Parse JSON response
            DynamicJsonDocument doc(1024);
            DeserializationError error = deserializeJson(doc, payload);
            
            if(!error) {
                // Extract follower count
                unsigned long followers = doc["followers_count"];
                counter = followers;
                
                String username = doc["username"].as<String>();
                String lastUpdated = doc["last_updated"].as<String>();
                
                Serial.printf("Updated follower count for %s: %lu (Last updated: %s)\n", 
                    username.c_str(), counter, lastUpdated.c_str());
                    
                success = true;
            } else {
                Serial.print("JSON parsing error: ");
                Serial.println(error.c_str());
            }
        } else {
            Serial.print("HTTP Error: ");
            Serial.println(httpResponseCode);
        }
        
        // Print connection details for debugging
        Serial.print("Connection close status: ");
        http.end();
        Serial.println("HTTP connection closed");
    } else {
        Serial.println("WiFi not connected, can't update follower count");
        Serial.print("WiFi status: ");
        Serial.println(WiFi.status());
    }
    
    return success;
}

/**
 * @brief Update the counter if enough time has passed
 * @return True if counter was updated
 */
bool updateCounter() {
    unsigned long currentMillis = millis();
    
    // Check if it's time to update the counter
    if (currentMillis - lastCounterUpdate >= COUNTER_UPDATE_INTERVAL) {
        // Save the last update time
        lastCounterUpdate = currentMillis;
        
        // Store the previous counter value for comparison
        prevCounter = counter;
        
        // Fetch updated follower count from API
        bool updated = fetchCounterFromAPI();
        
        // Debug info
        if(updated) {
            Serial.printf("Counter updated from API to: %lu at time %lu ms\n", counter, currentMillis);
        } else {
            Serial.println("Failed to update counter from API, using previous value");
        }
        
        return updated;
    }
    
    return false;
}

/**
 * @brief Draw a single digit with the specified color
 * @param digit The digit character to draw (0-9)
 * @param x X-position to draw at
 * @param y Y-position to draw at
 * @param textSize Size of the text
 * @param color Color to use for drawing
 */
void drawDigit(char digit, int16_t x, int16_t y, uint8_t textSize, uint16_t color) {
    matrix->setCursor(x, y);
    matrix->setTextColor(color);
    matrix->setTextSize(textSize);
    
    // Draw the single digit
    char digitStr[2] = {digit, '\0'};
    matrix->print(digitStr);
}

/**
 * @brief Display the counter on the matrix
 */
void displayCounter() {
    // Clear the screen before drawing
    matrix->clearScreen();
    
    // Convert the counter to a string with leading zeros
    char counterStr[20];
    sprintf(counterStr, "%0*lu", COUNTER_DIGITS, counter);
    
    // Set text properties
    uint8_t textSize = 2; // Base text size
    matrix->setTextWrap(false);
    
    // Calculate width of each digit at this size (approximately 5*textSize pixels)
    const uint16_t digitWidth = 5 * textSize;
    const uint16_t digitSpacing = 1; // Spacing between digits
    
    // Calculate total width of counter display
    uint16_t totalWidth = (COUNTER_DIGITS * digitWidth) + ((COUNTER_DIGITS - 1) * digitSpacing);
    
    // Center the counter string horizontally and vertically
    int16_t startX = (PANE_WIDTH - totalWidth) / 2;
    int16_t startY = (PANE_HEIGHT - (8 * textSize)) / 2;  // 8 is height of size 1 text
    
    // Draw each digit with the solid purple-blue color
    for(uint8_t i = 0; i < COUNTER_DIGITS; i++) {
        // Calculate position for this digit
        int16_t digitX = startX + i * (digitWidth + digitSpacing);
        
        // Draw the digit with the fixed color
        drawDigit(counterStr[i], digitX, startY, textSize, COUNTER_COLOR);
    }
}

/**
 * @brief Get the current counter value
 * @return Current counter value
 */
unsigned long getCounterValue() {
    return counter;
}

/**
 * @brief Display an SVG icon on the matrix
 * @param iconData Array containing the SVG icon data (24x24 pixels)
 * @param primaryColor The primary color for the icon
 * @param secondaryColor The secondary color for the icon
 * @param x X position to display the icon (top left corner)
 * @param y Y position to display the icon (top left corner)
 */
void displayIcon(const uint8_t* iconData, uint16_t primaryColor, uint16_t secondaryColor, int16_t x, int16_t y) {
    // Each byte represents 8 pixels in the iconData
    // The 24x24 icon requires 24*24/8 = 72 bytes of data
    const uint8_t iconWidth = 24;
    const uint8_t iconHeight = 24;
    
    // Loop through each row of the icon
    for (uint8_t row = 0; row < iconHeight; row++) {
        // Loop through each column of the icon
        for (uint8_t col = 0; col < iconWidth; col++) {
            // Calculate which byte and bit contain the pixel data
            uint16_t byteIndex = (row * iconWidth + col) / 8;
            uint8_t bitIndex = 7 - ((row * iconWidth + col) % 8); // MSB first
            
            // Check if the bit is set (1) or not (0)
            bool isSet = (iconData[byteIndex] & (1 << bitIndex)) > 0;
            
            // Calculate pixel position on the matrix
            int16_t pixelX = x + col;
            int16_t pixelY = y + row;
            
            // Only draw if within matrix bounds
            if (pixelX >= 0 && pixelX < PANE_WIDTH && pixelY >= 0 && pixelY < PANE_HEIGHT) {
                // If bit is 1, use primary color, otherwise use secondary color
                uint16_t pixelColor = isSet ? primaryColor : secondaryColor;
                
                // Draw the pixel only if non-transparent (assuming 0 is transparent)
                if (pixelColor != 0) {
                    matrix->drawPixel(pixelX, pixelY, pixelColor);
                }
            }
        }
    }
}