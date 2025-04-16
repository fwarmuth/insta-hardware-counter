#ifndef RANDOM_POSITION_ANIMATION_H
#define RANDOM_POSITION_ANIMATION_H

#include "animation_base.h"

/**
 * @brief Animation that displays counter at random positions
 */
class RandomPositionAnimation : public AnimationBase {
public:
    /**
     * @brief Constructor with configurable duration
     * @param durationMs Animation duration in milliseconds
     */
    RandomPositionAnimation(unsigned long durationMs = 10000);
    
    /**
     * @brief Draw the counter at a random position
     * @param counter Current counter value to display
     * @return True if animation needs to be refreshed
     */
    virtual bool draw(unsigned long counter) override;

private:
    int16_t posX;      // X position for the counter
    int16_t posY;      // Y position for the counter
    
    /**
     * @brief Choose a new random position that ensures counter visibility
     * @param counterWidth Width of the counter in pixels
     * @param counterHeight Height of the counter in pixels
     */
    void setRandomPosition(uint16_t counterWidth, uint16_t counterHeight);
};

#endif // RANDOM_POSITION_ANIMATION_H