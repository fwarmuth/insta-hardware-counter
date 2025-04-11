#include "main.h"
#include "matrix_config.h"
#include "counter.h"
#include <Arduino.h>
#include <SPIFFS.h>
#include "instagram_logo.h"

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
    
    // Initialize the LED matrix
    initMatrix();
    
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
    // Update counter if needed and refresh display
    if (updateCounter()) {
        // Clear the display
        matrix->clearScreen();
        
        // Instagram brand colors
        uint16_t primaryColor = matrix->color565(255, 255, 255);   // Instagram pink
        uint16_t secondaryColor = matrix->color565(0, 0, 0); // Instagram purple

        // Display the Instagram logo in the center of the left half of the display
        int centerX = PANEL_WIDTH / 2;  // Center of left half of the panel
        int centerY = PANEL_HEIGHT / 2; // Center of the panel height
        displayBitmap(image_data, image_width, image_height, 0, 0, centerX, centerY, 3, true);
        
        delay(5000); // Small delay to allow for rendering
        
        // Display counter on the right side of the icon
        displayCounter();
    }
    
    // Small delay to prevent CPU hogging
    delay(100);
}
