#ifndef ANIMATION_BASE_H
#define ANIMATION_BASE_H

#include <Arduino.h>

// Counter display color
#define COUNTER_COLOR 0x4A1F // Purple-blue color in RGB565 format

/**
 * @brief Base class for all counter animations
 */
class AnimationBase {
public:
    /**
     * @brief Constructor with configurable duration
     * @param durationMs Animation duration in milliseconds
     */
    AnimationBase(unsigned long durationMs = 10000);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~AnimationBase() {}
    
    /**
     * @brief Draw the counter animation
     * @param counter Current counter value to display
     * @return True if animation needs to be refreshed
     */
    virtual bool draw(unsigned long counter) = 0;
    
    /**
     * @brief Check if animation cycle is complete
     * @return True if animation duration has elapsed
     */
    bool isComplete();
    
    /**
     * @brief Reset the animation timer
     */
    void reset();
    
    /**
     * @brief Set the animation duration
     * @param durationMs New duration in milliseconds
     */
    void setDuration(unsigned long durationMs);

protected:
    unsigned long startTime;      // Animation start timestamp
    unsigned long duration;       // Animation duration in milliseconds
    bool firstDraw;              // Flag for first draw call
};

#endif // ANIMATION_BASE_H