#include "main.h"
#include "matrix_config.h"
#include "counter.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include "instagram_logo.h"
#include "wifi_manager.h"
#include "animations/animation_manager.h"

// Global animation manager instance
AnimationManager animationManager;

/**
 * @brief Setup function called once at startup
 */
void setup() {
    Serial.begin(BAUD_RATE);
    Serial.println("Starting counter application...");
    
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS initialization failed.");
    } else {
        Serial.println("SPIFFS initialized successfully.");
    }
    
    initMatrix();
    
    // Initialize WiFi connection with fallback to captive portal
    initWiFiWithCaptivePortal();
    
    // Initialize OTA after WiFi is connected
    // OTA is now initialized in initWiFiWithCaptivePortal() if WiFi connects successfully
    
    initCounter();
    
    // Initialize animations
    initAnimations();
    
    Serial.println("Initialization complete.");
}

/**
 * @brief Initialize the animation system
 */
void initAnimations() {
    // Initialize animations with durations set in animation_config.h
    animationManager.init();
    Serial.println("Animations initialized");
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
    
    // Handle captive portal if active, otherwise maintain WiFi connection
    if (!handleCaptivePortal()) {
        // Only check WiFi if captive portal is not active
        checkAndMaintainWiFi();
        
        // Update counter data using non-blocking approach - only if WiFi is connected
        if (WiFi.status() == WL_CONNECTED) {
            // First, check if we need to start a new request
            bool fetchStarted = checkCounterUpdateTime();
            if (fetchStarted) {
                Serial.println("Counter update initiated");
            }
            
            // Then, check if any in-progress request has completed
            APIRequestState state = getAPIRequestState();
            if (state == API_REQUEST_COMPLETE) {
                bool processed = processAsyncCounterFetch();
                if (processed) {
                    Serial.println("Counter updated");
                }
            }
        }
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
    // Use animation manager to draw the counter with the current animation style
    bool needsRefresh = animationManager.update(getCounterValue());
    if (needsRefresh) {
        // Animation state changed and needs a refresh
        Serial.println("Animation refreshed");
    }
    
    // Update status indicator with both WiFi and counter status
    bool wifiConnected = WiFi.status() == WL_CONNECTED;
    updateStatusIndicator(wifiConnected, isLastRequestSuccessful());
}

/**
 * @brief Manage the loop timing and log performance
 * 
 * @param startMillis Time when loop started
 */
void manageLoopTiming(unsigned long startMillis) {
    unsigned long elapsedTime = millis() - startMillis;
    
    // Add delay if needed
    if (elapsedTime < REFRESH_INTERVAL) {
        delay(REFRESH_INTERVAL - elapsedTime);
    } else {
        Serial.printf("Loop took longer than %dms, skipping delay\n", REFRESH_INTERVAL);
    }
    
    // Log performance occasionally
    if (loopCounter % 1000 == 0) {
        Serial.printf("Loop counter: %lu\n", loopCounter);
        Serial.printf("Loop took: %lu ms\n", millis() - startMillis);
    }
}
