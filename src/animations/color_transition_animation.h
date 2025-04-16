#ifndef COLOR_TRANSITION_ANIMATION_H
#define COLOR_TRANSITION_ANIMATION_H

#include "animation_base.h"

/**
 * @brief Animation that displays the counter with a continuous color transition
 */
class ColorTransitionAnimation : public AnimationBase {
public:
    /**
     * @brief Constructor with configurable duration
     * @param durationMs Animation duration in milliseconds
     * @param colorTransitionDurationMs Color transition duration in milliseconds
     */
    ColorTransitionAnimation(unsigned long durationMs = 10000, unsigned long colorTransitionDurationMs = 5000);
    
    /**
     * @brief Draw the counter with color transition
     * @param counter Current counter value to display
     * @return True if animation needs to be refreshed
     */
    virtual bool draw(unsigned long counter) override;
    
    /**
     * @brief Set the color transition duration
     * @param durationMs Color transition duration in milliseconds
     */
    void setColorTransitionDuration(unsigned long durationMs);
    
    /**
     * @brief Reset the animation
     * Overrides the base reset to also generate new colors
     */
    virtual void reset() override;

private:
    uint16_t startColor;       // Starting color for transition
    uint16_t targetColor;      // Target color for transition
    unsigned long colorTransitionDuration;  // Duration of color transition
    
    /**
     * @brief Generate a random color using the color wheel
     * @return Random 16-bit color
     */
    uint16_t generateRandomColor();
    
    /**
     * @brief Calculate the current transition color
     * @return Current interpolated color
     */
    uint16_t getCurrentColor();
};

#endif // COLOR_TRANSITION_ANIMATION_H