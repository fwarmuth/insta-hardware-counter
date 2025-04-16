#include "color_transition_animation.h"
#include "matrix_config.h"
#include "counter.h"
#include "color_utils.h"

/**
 * @brief Constructor with configurable duration
 * @param durationMs Animation duration in milliseconds
 * @param colorTransitionDurationMs Color transition duration in milliseconds
 */
ColorTransitionAnimation::ColorTransitionAnimation(unsigned long durationMs, unsigned long colorTransitionDurationMs) 
    : AnimationBase(durationMs), colorTransitionDuration(colorTransitionDurationMs) {
    // Generate initial colors
    startColor = generateRandomColor();
    targetColor = generateRandomColor();
}

/**
 * @brief Draw the counter with color transition
 * @param counter Current counter value to display
 * @return True if animation needs to be refreshed
 */
bool ColorTransitionAnimation::draw(unsigned long counter) {
    // Calculate current color based on elapsed time
    uint16_t currentColor = getCurrentColor();
    
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
    
    // Draw each digit with the current transition color
    for(uint8_t i = 0; i < COUNTER_DIGITS; i++) {
        int16_t digitX = startX + i * (digitWidth + digitSpacing);
        drawDigit(counterStr[i], digitX, startY, textSize, currentColor);
    }
    
    // Animation needs to refresh on each frame to update the color
    return true;
}

/**
 * @brief Set the color transition duration
 * @param durationMs Color transition duration in milliseconds
 */
void ColorTransitionAnimation::setColorTransitionDuration(unsigned long durationMs) {
    colorTransitionDuration = durationMs;
}

/**
 * @brief Reset the animation
 * Overrides the base reset to also generate new colors
 */
void ColorTransitionAnimation::reset() {
    AnimationBase::reset(); // Call parent reset
    
    // When resetting, the previous target color becomes the new start color
    startColor = targetColor;
    // And we generate a new target color
    targetColor = generateRandomColor();
}

/**
 * @brief Generate a random color using the color wheel
 * @return Random 16-bit color
 */
uint16_t ColorTransitionAnimation::generateRandomColor() {
    // Use the color wheel with a random position (0-255)
    return colorWheel(random(256));
}

/**
 * @brief Calculate the current transition color
 * @return Current interpolated color
 */
uint16_t ColorTransitionAnimation::getCurrentColor() {
    unsigned long elapsed = millis() - startTime;
    
    // Use a shorter duration for the color transition if specified
    unsigned long effectiveDuration = (colorTransitionDuration > 0 && colorTransitionDuration < duration) 
        ? colorTransitionDuration
        : duration;
    
    // Cap at the effective duration
    if (elapsed > effectiveDuration) {
        elapsed = effectiveDuration;
    }
    
    // Calculate progress (0.0 - 1.0)
    float progress = (float)elapsed / effectiveDuration;
    
    // Extract RGB components from start and target colors
    uint8_t startR = (startColor >> 11) & 0x1F;
    uint8_t startG = (startColor >> 5) & 0x3F;
    uint8_t startB = startColor & 0x1F;
    
    uint8_t targetR = (targetColor >> 11) & 0x1F;
    uint8_t targetG = (targetColor >> 5) & 0x3F;
    uint8_t targetB = targetColor & 0x1F;
    
    // Interpolate between start and target colors
    uint8_t currentR = startR + (targetR - startR) * progress;
    uint8_t currentG = startG + (targetG - startG) * progress;
    uint8_t currentB = startB + (targetB - startB) * progress;
    
    // Construct the interpolated color
    return (currentR << 11) | (currentG << 5) | currentB;
}