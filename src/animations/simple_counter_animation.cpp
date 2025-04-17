#include "simple_counter_animation.h"
#include "matrix_config.h"
#include "counter.h"
#include "color_utils.h"

/**
 * @brief Constructor with configurable duration and color
 * @param durationMs Animation duration in milliseconds
 * @param color Color to use for the counter
 */
SimpleCounterAnimation::SimpleCounterAnimation(unsigned long durationMs, uint16_t color) : 
    AnimationBase(durationMs),
    counterColor(color) {
}

/**
 * @brief Set the counter color
 * @param color New color for the counter
 */
void SimpleCounterAnimation::setColor(uint16_t color) {
    counterColor = color;
}

/**
 * @brief Draw the simple counter animation
 * @param counter Current counter value to display
 * @return True if animation needs to be refreshed
 */
bool SimpleCounterAnimation::draw(unsigned long counter) {
    // This is similar to the original displayCounter implementation
    // Convert the counter to a string with leading zeros
    char counterStr[20];
    sprintf(counterStr, "%0*lu", COUNTER_DIGITS, counter);
    
    // Set text properties
    uint8_t textSize = 2; // Base text size
    matrix->setTextWrap(false);
    
    // Calculate width of each digit and total width
    const uint16_t digitWidth = 5 * textSize;
    const uint16_t digitSpacing = 1;
    uint16_t totalWidth = (COUNTER_DIGITS * digitWidth) + ((COUNTER_DIGITS - 1) * digitSpacing);
    
    // Center the counter string horizontally and vertically
    int16_t startX = (PANE_WIDTH - totalWidth) / 2;
    int16_t startY = (PANE_HEIGHT - (8 * textSize)) / 2;
    
    // Draw each digit
    for(uint8_t i = 0; i < COUNTER_DIGITS; i++) {
        int16_t digitX = startX + i * (digitWidth + digitSpacing);
        drawDigit(counterStr[i], digitX, startY, textSize, counterColor);
    }
    
    // Animation only needs to refresh on first draw then stays static
    if (firstDraw) {
        firstDraw = false;
        return true;
    }
    
    return false;
}

/**
 * @brief Reset the animation timer and randomize color
 */
void SimpleCounterAnimation::reset() {
    // Call the parent class reset to handle timer reset
    AnimationBase::reset();
    
    // Randomize the counter color
    counterColor = colorWheel(random(0, 256));
}