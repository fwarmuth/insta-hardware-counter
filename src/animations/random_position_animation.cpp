#include "random_position_animation.h"
#include "matrix_config.h"
#include "counter.h"
#include "color_utils.h"

/**
 * @brief Constructor with configurable duration and color
 * @param durationMs Animation duration in milliseconds
 * @param color Color to use for the counter
 */
RandomPositionAnimation::RandomPositionAnimation(unsigned long durationMs, uint16_t color) : 
    AnimationBase(durationMs),
    posX(0),
    posY(0),
    counterColor(color) {
    // Initial position will be set on first draw
}

/**
 * @brief Set the counter color
 * @param color New color for the counter
 */
void RandomPositionAnimation::setColor(uint16_t color) {
    counterColor = color;
}

/**
 * @brief Draw the counter at a random position
 * @param counter Current counter value to display
 * @return True if animation needs to be refreshed
 */
bool RandomPositionAnimation::draw(unsigned long counter) {
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
    uint16_t totalHeight = 8 * textSize; // Height of the text
    
    // Set a new random position on first draw
    if (firstDraw) {
        setRandomPosition(totalWidth, totalHeight);
        firstDraw = false;
        return true;
    }
    
    // Draw each digit at the random position
    for(uint8_t i = 0; i < COUNTER_DIGITS; i++) {
        int16_t digitX = posX + i * (digitWidth + digitSpacing);
        drawDigit(counterStr[i], digitX, posY, textSize, counterColor);
    }
    
    return false;
}

/**
 * @brief Choose a new random position that ensures counter visibility
 * @param counterWidth Width of the counter in pixels
 * @param counterHeight Height of the counter in pixels
 */
void RandomPositionAnimation::setRandomPosition(uint16_t counterWidth, uint16_t counterHeight) {
    // Ensure the counter stays fully visible on screen
    int16_t maxX = PANE_WIDTH - counterWidth;
    int16_t maxY = PANE_HEIGHT - counterHeight;
    
    // Get random positions within safe bounds
    if (maxX > 0) {
        posX = random(0, maxX);
    } else {
        posX = 0;
    }
    
    if (maxY > 0) {
        posY = random(0, maxY);
    } else {
        posY = 0;
    }
    
    Serial.printf("Set random counter position to: (%d, %d)\n", posX, posY);
}

/**
 * @brief Reset the animation timer, position and randomize color
 */
void RandomPositionAnimation::reset() {
    // Call the parent class reset to handle timer reset
    AnimationBase::reset();
    
    // Randomize the counter color
    counterColor = colorWheel(random(0, 256));
    
    // Note: New random position will be set on the next draw() call when firstDraw is true
}