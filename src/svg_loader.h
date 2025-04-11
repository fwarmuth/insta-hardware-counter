#ifndef SVG_LOADER_H
#define SVG_LOADER_H

#include <Arduino.h>
#include "matrix_config.h"

/**
 * @brief Initialize the filesystem for storing SVG files
 * @return True if initialization was successful
 */
bool initSVGFileSystem();

/**
 * @brief Load an SVG file from SPIFFS and convert it to a bitmap
 * @param fileName Name of the SVG file in SPIFFS
 * @param iconData Pointer to array where bitmap data will be stored (must be at least 72 bytes)
 * @param maxSize Maximum size of the iconData array
 * @return True if loading and conversion was successful
 */
bool loadSVGFromFile(const char* fileName, uint8_t* iconData, size_t maxSize);

/**
 * @brief Display an SVG file directly from SPIFFS
 * @param fileName Name of the SVG file in SPIFFS
 * @param primaryColor The primary color for the icon (for '1' bits)
 * @param secondaryColor The secondary color for the icon (for '0' bits)
 * @param x X position to display the icon (top left corner)
 * @param y Y position to display the icon (top left corner)
 * @return True if loading and displaying was successful
 */
bool displaySVGFromFile(const char* fileName, uint16_t primaryColor, uint16_t secondaryColor, int16_t x, int16_t y);

#endif // SVG_LOADER_H