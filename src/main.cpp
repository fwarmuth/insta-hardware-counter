#include "main.h"
#include "matrix_config.h"
#include "counter.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include "instagram_logo.h"
#include <WiFi.h>

/**
 * @brief Connects to WiFi network
 * 
 * @return True if connection was successful
 */
bool connectToWiFi() {
    Serial.printf("Connecting to WiFi network: %s\n", WIFI_SSID);
    
    // Update indicator to show disconnected status
    updateWiFiStatusIndicator(false);
    
    // Set WiFi mode to station (client)
    WiFi.mode(WIFI_STA);
    
    // Start WiFi connection
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    // Set timeout for connection
    unsigned long connectionStartTime = millis();
    
    // Wait for connection or timeout
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - connectionStartTime > WIFI_CONNECT_TIMEOUT) {
            Serial.println("WiFi connection failed, timeout reached");
            updateWiFiStatusIndicator(false);
            return false;
        }
        
        delay(500);
        Serial.print(".");
    }
    
    // Print connection details
    Serial.println();
    Serial.println("WiFi connected successfully");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("Signal strength (RSSI): ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm");
    
    // Update indicator to show connected status
    updateWiFiStatusIndicator(true);
    
    return true;
}

/**
 * @brief Setup function called once at startup
 * 
 * Initializes serial communication, matrix display and counter
 */
void setup() {
    // Initialize serial communication
    Serial.begin(BAUD_RATE);
    Serial.println("Starting counter application...");
    
    // Initialize SPIFFS for loading image files
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
    } else {
        Serial.println("SPIFFS initialized successfully.");
    }
    
    // Initialize the LED matrix first so we can use the status indicator
    initMatrix();
    
    // Connect to WiFi - status indicator will be updated in connectToWiFi()
    if (!connectToWiFi()) {
        Serial.println("Continuing without WiFi connection");
    }
    
    // Initialize the counter
    initCounter();
    
    Serial.println("Initialization complete.");
}

/**
 * @brief Main program loop
 * 
 * Checks if counter needs updating and refreshes display when it does
 */
void loop() {
    // Check WiFi connection and reconnect if needed
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi connection lost, attempting to reconnect...");
        // Status indicator will be updated in connectToWiFi()
        connectToWiFi();
    } else {
        // Ensure status indicator shows connected
        updateWiFiStatusIndicator(true);
    }
    
    // Check if counter needs updating
    bool counterUpdated = updateCounter();
    
    if (counterUpdated) {
        Serial.println("Counter updated, refreshing display");
    }
    
    // Always update the counter display
    displayCounter();
    
    // Make sure WiFi status indicator is still visible after display refresh
    updateWiFiStatusIndicator(WiFi.status() == WL_CONNECTED);
    
    // Small delay to prevent CPU hogging
    delay(100);
}
