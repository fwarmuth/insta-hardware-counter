#ifndef SIMPLE_COUNTER_ANIMATION_H
#define SIMPLE_COUNTER_ANIMATION_H

#include "animation_base.h"

/**
 * @brief Simple animation that centers the counter on screen
 */
class SimpleCounterAnimation : public AnimationBase {
public:
    /**
     * @brief Constructor with configurable duration
     * @param durationMs Animation duration in milliseconds
     */
    SimpleCounterAnimation(unsigned long durationMs = 10000);
    
    /**
     * @brief Draw the simple counter animation
     * @param counter Current counter value to display
     * @return True if animation needs to be refreshed
     */
    virtual bool draw(unsigned long counter) override;
};

#endif // SIMPLE_COUNTER_ANIMATION_H