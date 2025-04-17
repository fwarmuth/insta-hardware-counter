#ifndef SIMPLE_COUNTER_ANIMATION_H
#define SIMPLE_COUNTER_ANIMATION_H

#include "animation_base.h"

/**
 * @brief Simple animation that centers the counter on screen
 */
class SimpleCounterAnimation : public AnimationBase {
public:
    /**
     * @brief Constructor with configurable duration and color
     * @param durationMs Animation duration in milliseconds
     * @param color Color to use for the counter (default: COUNTER_COLOR)
     */
    SimpleCounterAnimation(unsigned long durationMs = 10000, uint16_t color = COUNTER_COLOR);
    
    /**
     * @brief Draw the simple counter animation
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
     * @brief Reset the animation timer and randomize color
     */
    virtual void reset() override;

private:
    uint16_t counterColor; // Color for the counter display
};

#endif // SIMPLE_COUNTER_ANIMATION_H