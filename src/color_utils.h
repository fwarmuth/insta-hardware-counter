#ifndef COLOR_UTILS_H
#define COLOR_UTILS_H

#include <stdint.h>
#include "matrix_config.h"

/**
 * @brief Generate a color based on wheel position (0-255)
 * @param pos Position on the color wheel (0-255)
 * @return 16-bit color value
 */
uint16_t colorWheel(uint8_t pos);

#endif // COLOR_UTILS_H