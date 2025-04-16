#ifndef MAIN_H
#define MAIN_H

#include <Arduino.h>
#include "wifi_manager.h"

// Serial communication settings
#define BAUD_RATE 115200

// Application settings
#define REFRESH_INTERVAL 500 // Display refresh interval in milliseconds

/**
 * @brief Update the display with counter and status
 */
void updateDisplay();

/**
 * @brief Manage the loop timing and log performance
 * 
 * @param startMillis Time when loop started
 */
void manageLoopTiming(unsigned long startMillis);

/**
 * @brief Setup function called once at startup
 */
void setup();

/**
 * @brief Main program loop
 */
void loop();

#endif // MAIN_H