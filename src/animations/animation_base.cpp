#include "animation_base.h"

/**
 * @brief Constructor with configurable duration
 * @param durationMs Animation duration in milliseconds
 */
AnimationBase::AnimationBase(unsigned long durationMs) : 
    startTime(millis()), 
    duration(durationMs),
    firstDraw(true) {
}

/**
 * @brief Check if animation cycle is complete
 * @return True if animation duration has elapsed
 */
bool AnimationBase::isComplete() {
    return (millis() - startTime) >= duration;
}

/**
 * @brief Reset the animation timer
 */
void AnimationBase::reset() {
    startTime = millis();
    firstDraw = true;
}

/**
 * @brief Set the animation duration
 * @param durationMs New duration in milliseconds
 */
void AnimationBase::setDuration(unsigned long durationMs) {
    duration = durationMs;
}