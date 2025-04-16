#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "animation_base.h"
#include "simple_counter_animation.h"
#include "random_position_animation.h"
#include "color_transition_animation.h"

// Animation styles enumeration
enum AnimationStyle {
    STYLE_SIMPLE_COUNTER = 0,
    STYLE_RANDOM_POSITION,
    
    // Add new styles here
    STYLE_COLOR_TRANSITION,
    
    STYLE_COUNT  // Always keep this as last item for tracking the total count
};

/**
 * @brief Manages animation styles and transitions
 */
class AnimationManager {
public:
    /**
     * @brief Constructor
     */
    AnimationManager();
    
    /**
     * @brief Destructor
     */
    ~AnimationManager();
    
    /**
     * @brief Initialize the animation manager
     */
    void init();
    
    /**
     * @brief Update the animation state and draw the current animation
     * @param counter Current counter value to display
     * @return True if animation was refreshed
     */
    bool update(unsigned long counter);
    
    /**
     * @brief Set a specific animation style
     * @param style The animation style to set
     */
    void setAnimationStyle(AnimationStyle style);
    
    /**
     * @brief Get the current animation style
     * @return Current animation style
     */
    AnimationStyle getCurrentStyle() const;
    
    /**
     * @brief Set duration for a specific animation style
     * @param style The animation style to configure
     * @param durationMs Duration in milliseconds
     */
    void setAnimationDuration(AnimationStyle style, unsigned long durationMs);

private:
    AnimationBase* animations[STYLE_COUNT];  // Array of animation instances
    AnimationStyle currentStyle;             // Current active animation style
    
    /**
     * @brief Switch to the next animation style
     */
    void nextAnimation();
};

#endif // ANIMATION_MANAGER_H