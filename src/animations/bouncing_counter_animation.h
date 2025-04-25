#ifndef BOUNCING_COUNTER_ANIMATION_H
#define BOUNCING_COUNTER_ANIMATION_H

#include "animation_base.h"

/**
 * @brief Animation that makes the counter bounce from edge to edge like a screensaver
 */
class BouncingCounterAnimation : public AnimationBase {
public:
    /**
     * @brief Constructor with configurable duration and color
     * @param durationMs Animation duration in milliseconds
     * @param color Color to use for the counter (default: COUNTER_COLOR)
     */
    BouncingCounterAnimation(unsigned long durationMs = 10000, uint16_t color = COUNTER_COLOR);
    
    /**
     * @brief Draw the bouncing counter animation
     * @param counter Current counter value to display
     * @return True if animation needs to be refreshed
     */
    virtual bool draw(unsigned long counter) override;
    
    /**
     * @brief Set the counter color
     * @param color New color for the counter
     */
    void setColor(uint16_t color);
    
    /**
     * @brief Reset the animation timer and position
     */
    virtual void reset() override;

private:
    uint16_t counterColor;    // Color for the counter display
    int16_t posX;            // Current X position
    int16_t posY;            // Current Y position
    int8_t directionX;       // X movement direction (1 or -1)
    int8_t directionY;       // Y movement direction (1 or -1)
    int16_t speedX;          // X speed in pixels per update
    int16_t speedY;          // Y speed in pixels per update
};

#endif // BOUNCING_COUNTER_ANIMATION_H