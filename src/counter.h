#ifndef COUNTER_H
#define COUNTER_H

#include <Arduino.h>

// Counter configuration
#define COUNTER_UPDATE_INTERVAL 10000  // 10 seconds in milliseconds
#define COUNTER_DIGITS 5               // Number of digits to display

// Function declarations
/**
 * @brief Initialize the counter
 */
void initCounter();

/**
 * @brief Update the counter if enough time has passed
 * @return True if counter was updated
 */
bool updateCounter();

/**
 * @brief Display the counter on the matrix
 */
void displayCounter();

/**
 * @brief Display an SVG icon on the matrix
 * @param iconData Array containing the SVG icon data (24x24 pixels)
 * @param primaryColor The primary color for the icon
 * @param secondaryColor The secondary color for the icon
 * @param x X position to display the icon (top left corner)
 * @param y Y position to display the icon (top left corner)
 */
void displayIcon(const uint8_t* iconData, uint16_t primaryColor, uint16_t secondaryColor, int16_t x, int16_t y);

/**
 * @brief Get the current counter value
 * @return Current counter value
 */
unsigned long getCounterValue();

#endif // COUNTER_H