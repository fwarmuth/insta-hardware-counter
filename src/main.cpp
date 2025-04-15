#include "main.h"
#include "matrix_config.h"
#include "counter.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include "instagram_logo.h"
#include "wifi_manager.h"

/**
 * @brief Setup function called once at startup
 */
void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("Starting counter application...");
    
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed!");
    } else {
        Serial.println("SPIFFS initialized successfully.");
    }
    
    initMatrix();
    
    // Initialize WiFi connection
    initWiFi();
    
    // Initialize OTA after WiFi is connected
    initOTA();
    
    initCounter();
    Serial.println("Initialization complete.");
}

/**
 * @brief Main program loop
 */
unsigned long loopCounter = 0;

void loop() {
    loopCounter++;
    unsigned long startMillis = millis();
    
    // Handle OTA updates
    handleOTA();
    
    // Handle WiFi connection
    checkAndMaintainWiFi();
    
    // Update counter data if needed
    bool counterUpdated = updateCounter();
    if (counterUpdated) {
        Serial.println("Counter updated");
    }
    
    // Refresh display
    updateDisplay();
    
    // Rate limit the loop execution
    manageLoopTiming(startMillis);
}

/**
 * @brief Update the display with counter and status
 */
void updateDisplay() {
    matrix->clearScreen();
    displayCounter();
    updateWiFiStatusIndicator(WiFi.status() == WL_CONNECTED);
}

/**
 * @brief Manage the loop timing and log performance
 * 
 * @param startMillis Time when loop started
 */
void manageLoopTiming(unsigned long startMillis) {
    unsigned long elapsedTime = millis() - startMillis;
    
    // Add delay if needed
    if (elapsedTime < 50) {
        delay(50 - elapsedTime);
    } else {
        Serial.println("Loop took longer than 50ms, skipping delay");
    }
    
    // Log performance occasionally
    if (loopCounter % 1000 == 0) {
        Serial.printf("Loop counter: %lu\n", loopCounter);
        Serial.printf("Loop took: %lu ms\n", millis() - startMillis);
    }
}
