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
 * @brief Initialize the animation manager
 */
void AnimationManager::init() {
    // Create animation instances
    if (animations[STYLE_SIMPLE_COUNTER] == nullptr) {
        animations[STYLE_SIMPLE_COUNTER] = new SimpleCounterAnimation(10000); // 10 seconds
    }
    
    if (animations[STYLE_RANDOM_POSITION] == nullptr) {
        animations[STYLE_RANDOM_POSITION] = new RandomPositionAnimation(10000); // 10 seconds
    }
    
    if (animations[STYLE_COLOR_TRANSITION] == nullptr) {
        animations[STYLE_COLOR_TRANSITION] = new ColorTransitionAnimation(10000, 5000); // 10 seconds animation, 5 seconds color transition
    }
    
    // Initialize with the first style
    currentStyle = STYLE_SIMPLE_COUNTER;
    
    Serial.println("Animation manager initialized");
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
    // Cycle to the next available style
    AnimationStyle nextStyle = static_cast<AnimationStyle>((currentStyle + 1) % STYLE_COUNT);
    
    // Set the next style
    setAnimationStyle(nextStyle);
}