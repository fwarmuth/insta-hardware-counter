#include "bouncing_counter_animation.h"
#include "matrix_config.h"
#include "counter.h"
#include "color_utils.h"

/**
 * @brief Constructor with configurable duration and color
 * @param durationMs Animation duration in milliseconds
 * @param color Color to use for the counter
 */
BouncingCounterAnimation::BouncingCounterAnimation(unsigned long durationMs, uint16_t color) : 
    AnimationBase(durationMs),
    counterColor(color) {
    reset();
}

/**
 * @brief Set the counter color
 * @param color New color for the counter
 */
void BouncingCounterAnimation::setColor(uint16_t color) {
    counterColor = color;
}

/**
 * @brief Draw the bouncing counter animation
 * @param counter Current counter value to display
 * @return True if animation needs to be refreshed
 */
bool BouncingCounterAnimation::draw(unsigned long counter) {
    if (firstDraw) {
        firstDraw = false;
    }
    
    // Clear the display for the next frame
    matrix->fillScreen(0);
    
    // Convert the counter to a string with leading zeros
    char counterStr[20];
    sprintf(counterStr, "%0*lu", COUNTER_DIGITS, counter);
    
    // Set text properties
    uint8_t textSize = 2; // Base text size
    matrix->setTextWrap(false);
    
    // Calculate width and height of the counter
    const uint16_t digitWidth = 5 * textSize;
    const uint16_t digitSpacing = 1;
    uint16_t totalWidth = (COUNTER_DIGITS * digitWidth) + ((COUNTER_DIGITS - 1) * digitSpacing);
    uint16_t totalHeight = 8 * textSize; // Character height (8) * text size
    
    // Update position based on direction
    posX += directionX * speedX;
    posY += directionY * speedY;
    
    // Check for collision with edges and bounce
    if (posX <= 0) {
        posX = 0;
        directionX = 1; // Reverse direction
        counterColor = colorWheel(random(0, 256)); // Change color on bounce
    } else if (posX + totalWidth >= PANE_WIDTH) {
        posX = PANE_WIDTH - totalWidth;
        directionX = -1; // Reverse direction
        counterColor = colorWheel(random(0, 256)); // Change color on bounce
    }
    
    if (posY <= 0) {
        posY = 0;
        directionY = 1; // Reverse direction
        counterColor = colorWheel(random(0, 256)); // Change color on bounce
    } else if (posY + totalHeight >= PANE_HEIGHT) {
        posY = PANE_HEIGHT - totalHeight;
        directionY = -1; // Reverse direction
        counterColor = colorWheel(random(0, 256)); // Change color on bounce
    }
    
    // Draw each digit
    for(uint8_t i = 0; i < COUNTER_DIGITS; i++) {
        int16_t digitX = posX + i * (digitWidth + digitSpacing);
        drawDigit(counterStr[i], digitX, posY, textSize, counterColor);
    }
    
    // Always return true to refresh the display for the animation
    return true;
}

/**
 * @brief Reset the animation timer and position
 */
void BouncingCounterAnimation::reset() {
    // Call the parent class reset to handle timer reset
    AnimationBase::reset();
    
    // Randomize the counter color
    counterColor = colorWheel(random(0, 256));
    
    // Initialize the position to a random location on the display
    uint8_t textSize = 2;
    const uint16_t digitWidth = 5 * textSize;
    const uint16_t digitSpacing = 1;
    uint16_t totalWidth = (COUNTER_DIGITS * digitWidth) + ((COUNTER_DIGITS - 1) * digitSpacing);
    uint16_t totalHeight = 8 * textSize;
    
    posX = random(0, PANE_WIDTH - totalWidth);
    posY = random(0, PANE_HEIGHT - totalHeight);
    
    // Initialize direction randomly but ensure it's not zero
    directionX = random(0, 2) ? 1 : -1;
    directionY = random(0, 2) ? 1 : -1;
    
    // Set initial speed (can be adjusted for the desired effect)
    speedX = random(1, 3);
    speedY = random(1, 3);
}