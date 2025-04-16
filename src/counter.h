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
 * @brief Fetch follower count from Instagram API
 * @return True if successful
 */
bool fetchCounterFromAPI();

/**
 * @brief Log HTTP error codes with descriptions
 * @param httpResponseCode The error code to log
 */
void logHttpError(int httpResponseCode);

/**
 * @brief Update the counter if enough time has passed
 * @return True if counter was updated
 */
bool updateCounter();

/**
 * @brief Draw a single digit with the specified color
 * @param digit The digit character to draw (0-9)
 * @param x X-position to draw at
 * @param y Y-position to draw at
 * @param textSize Size of the text
 * @param color Color to use for drawing
 */
void drawDigit(char digit, int16_t x, int16_t y, uint8_t textSize, uint16_t color);

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

/**
 * @brief Get the status of the last API request
 * @return True if the last API request was successful, false otherwise
 */
bool isLastRequestSuccessful();

#endif // COUNTER_H