#include "color_utils.h"

/**
 * @brief Generate a color based on wheel position (0-255)
 * 
 * Provides a smooth transition through the color spectrum
 * based on the input position.
 * 
 * @param pos Position on the color wheel (0-255)
 * @return 16-bit color value
 */
uint16_t colorWheel(uint8_t pos) {
    if(pos < 85) {
        return matrix->color565(pos * 3, 255 - pos * 3, 0);
    } else if(pos < 170) {
        pos -= 85;
        return matrix->color565(255 - pos * 3, 0, pos * 3);
    } else {
        pos -= 170;
        return matrix->color565(0, pos * 3, 255 - pos * 3);
    }
}