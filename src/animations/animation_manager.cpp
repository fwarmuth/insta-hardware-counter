#include "animation_manager.h"
#include <Arduino.h>

/**
 * @brief Constructor
 */
AnimationManager::AnimationManager() : currentStyle(STYLE_SIMPLE_COUNTER) {
    // Initialize array with nullptrs
    for (int i = 0; i < STYLE_COUNT; i++) {
        animations[i] = nullptr;
    }
}

/**
 * @brief Destructor
 */
AnimationManager::~AnimationManager() {
    // Clean up any allocated animations
    for (int i = 0; i < STYLE_COUNT; i++) {
        if (animations[i] != nullptr) {
            delete animations[i];
            animations[i] = nullptr;
        }
    }
}

/**
 * @brief Check if an animation is enabled in configuration
 * @param style The animation style to check
 * @return True if the animation is enabled
 */
bool AnimationManager::isAnimationEnabled(AnimationStyle style) {
    // This implementation is kept for backward compatibility but isn't used
    // by the ANIM_ENABLED macro anymore
    switch (style) {
        case STYLE_SIMPLE_COUNTER:
            return ENABLE_SIMPLE_COUNTER;
        case STYLE_RANDOM_POSITION:
            return ENABLE_RANDOM_POSITION;
        case STYLE_COLOR_TRANSITION:
            return ENABLE_COLOR_TRANSITION;
        case STYLE_BOUNCING_COUNTER:
            return ENABLE_BOUNCING_COUNTER;
        default:
            return false;
    }
}

/**
 * @brief Initialize the animation manager
 */
void AnimationManager::init() {
    // Create animation instances only for enabled animations with duration from config
    if (ANIM_ENABLED(STYLE_SIMPLE_COUNTER) && animations[STYLE_SIMPLE_COUNTER] == nullptr) {
        animations[STYLE_SIMPLE_COUNTER] = new SimpleCounterAnimation(DURATION_SIMPLE_COUNTER);
    }
    
    if (ANIM_ENABLED(STYLE_RANDOM_POSITION) && animations[STYLE_RANDOM_POSITION] == nullptr) {
        animations[STYLE_RANDOM_POSITION] = new RandomPositionAnimation(DURATION_RANDOM_POSITION);
    }
    
    if (ANIM_ENABLED(STYLE_COLOR_TRANSITION) && animations[STYLE_COLOR_TRANSITION] == nullptr) {
        animations[STYLE_COLOR_TRANSITION] = new ColorTransitionAnimation(DURATION_COLOR_TRANSITION, DURATION_COLOR_TRANSITION);
    }
    
    if (ANIM_ENABLED(STYLE_BOUNCING_COUNTER) && animations[STYLE_BOUNCING_COUNTER] == nullptr) {
        animations[STYLE_BOUNCING_COUNTER] = new BouncingCounterAnimation(DURATION_BOUNCING_COUNTER);
    }
    
    // Initialize with the first enabled style
    bool foundEnabled = false;
    for (int i = 0; i < STYLE_COUNT; i++) {
        if (ANIM_ENABLED(static_cast<AnimationStyle>(i)) && animations[i] != nullptr) {
            currentStyle = static_cast<AnimationStyle>(i);
            foundEnabled = true;
            break;
        }
    }
    
    if (!foundEnabled) {
        Serial.println("Warning: No animations are enabled!");
    } else {
        Serial.println("Animation manager initialized");
    }
}

/**
 * @brief Find the next enabled animation style
 * @param startStyle The style to start searching from
 * @return The next enabled animation style
 */
AnimationStyle AnimationManager::findNextEnabledAnimation(AnimationStyle startStyle) {
    AnimationStyle style = startStyle;
    
    // Loop through all styles (max STYLE_COUNT times) to find an enabled one
    for (int i = 0; i < STYLE_COUNT; i++) {
        // Move to the next style (wrapping around if necessary)
        style = static_cast<AnimationStyle>((style + 1) % STYLE_COUNT);
        
        // Return this style if it's enabled and initialized
        if (ANIM_ENABLED(style) && animations[style] != nullptr) {
            return style;
        }
    }
    
    // If we got here, no enabled animations were found
    // Return the original style as fallback
    return startStyle;
}

/**
 * @brief Update the animation state and draw the current animation
 * @param counter Current counter value to display
 * @return True if animation was refreshed
 */
bool AnimationManager::update(unsigned long counter) {
    // Check for null pointer
    if (animations[currentStyle] == nullptr) {
        Serial.printf("Error: Animation style %d not initialized\n", currentStyle);
        return false;
    }
    
    // Check if current animation is complete
    if (animations[currentStyle]->isComplete()) {
        Serial.printf("Animation style %d completed, switching to next\n", currentStyle);
        nextAnimation();
        return true; // Force refresh when switching animations
    }
    
    // Draw the current animation
    return animations[currentStyle]->draw(counter);
}

/**
 * @brief Set a specific animation style
 * @param style The animation style to set
 */
void AnimationManager::setAnimationStyle(AnimationStyle style) {
    if (style < 0 || style >= STYLE_COUNT) {
        Serial.printf("Invalid animation style: %d\n", style);
        return;
    }
    
    if (!ANIM_ENABLED(style)) {
        Serial.printf("Animation style %d is disabled in configuration\n", style);
        return;
    }
    
    if (animations[style] == nullptr) {
        Serial.printf("Animation style %d not initialized\n", style);
        return;
    }
    
    currentStyle = style;
    animations[style]->reset(); // Reset the animation timer
    
    Serial.printf("Switched to animation style: %d\n", style);
}

/**
 * @brief Get the current animation style
 * @return Current animation style
 */
AnimationStyle AnimationManager::getCurrentStyle() const {
    return currentStyle;
}

/**
 * @brief Set duration for a specific animation style
 * @param style The animation style to configure
 * @param durationMs Duration in milliseconds
 */
void AnimationManager::setAnimationDuration(AnimationStyle style, unsigned long durationMs) {
    if (style < 0 || style >= STYLE_COUNT) {
        Serial.printf("Invalid animation style: %d\n", style);
        return;
    }
    
    if (!ANIM_ENABLED(style)) {
        Serial.printf("Animation style %d is disabled in configuration\n", style);
        return;
    }
    
    if (animations[style] == nullptr) {
        Serial.printf("Animation style %d not initialized\n", style);
        return;
    }
    
    animations[style]->setDuration(durationMs);
    Serial.printf("Set duration for style %d to %lu ms\n", style, durationMs);
}

/**
 * @brief Switch to the next animation style
 */
void AnimationManager::nextAnimation() {
    // Find the next enabled animation
    AnimationStyle nextStyle = findNextEnabledAnimation(currentStyle);
    
    // If we couldn't find another enabled animation, just stay on the current one
    if (nextStyle == currentStyle) {
        animations[currentStyle]->reset(); // Reset the current animation
        Serial.println("No other enabled animations found, resetting current");
    } else {
        // Set the next style
        setAnimationStyle(nextStyle);
    }
}