#ifndef ANIMATION_MANAGER_H
#define ANIMATION_MANAGER_H

#include "animation_base.h"
#include "simple_counter_animation.h"
#include "random_position_animation.h"
#include "color_transition_animation.h"
#include "bouncing_counter_animation.h"
#include "animation_config.h"

// Animation styles enumeration
enum AnimationStyle {
    STYLE_SIMPLE_COUNTER = 0,
    STYLE_RANDOM_POSITION,
    STYLE_COLOR_TRANSITION,
    STYLE_BOUNCING_COUNTER,
    
    STYLE_COUNT  // Always keep this as last item for tracking the total count
};

// Helper macro to check if an animation is enabled
#define ANIM_ENABLED(style) ( \
    ((style) == STYLE_SIMPLE_COUNTER    ? ANIM_ENABLED_SIMPLE_COUNTER    : \
    ((style) == STYLE_RANDOM_POSITION   ? ANIM_ENABLED_RANDOM_POSITION   : \
    ((style) == STYLE_COLOR_TRANSITION  ? ANIM_ENABLED_COLOR_TRANSITION  : \
    ((style) == STYLE_BOUNCING_COUNTER  ? ANIM_ENABLED_BOUNCING_COUNTER  : 0)))) \
)

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

    /**
     * @brief Check if an animation is enabled in configuration
     * @param style The animation style to check
     * @return True if the animation is enabled
     */
    static bool isAnimationEnabled(AnimationStyle style);

private:
    AnimationBase* animations[STYLE_COUNT];  // Array of animation instances
    AnimationStyle currentStyle;             // Current active animation style
    
    /**
     * @brief Switch to the next animation style
     */
    void nextAnimation();
    
    /**
     * @brief Find the next enabled animation style
     * @param startStyle The style to start searching from
     * @return The next enabled animation style
     */
    AnimationStyle findNextEnabledAnimation(AnimationStyle startStyle);
};

#endif // ANIMATION_MANAGER_H