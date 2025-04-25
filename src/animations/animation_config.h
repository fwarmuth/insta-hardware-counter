#ifndef ANIMATION_CONFIG_H
#define ANIMATION_CONFIG_H

/**
 * Animation Configuration
 * 
 * This file allows you to easily configure all animation parameters:
 * - Enable/disable specific animations (set to 1 to enable, 0 to disable)
 * - Set the duration for each animation (in milliseconds)
 * 
 * When an animation is disabled:
 * - It will not be initialized
 * - It will not consume memory
 * - It will be skipped in the animation rotation
 */

// -----------------------------------------------------
// Animation Enable/Disable Configuration
// -----------------------------------------------------
#define ENABLE_SIMPLE_COUNTER      0   // Simple counter display
#define ENABLE_RANDOM_POSITION     1   // Counter with random position changes
#define ENABLE_COLOR_TRANSITION    1   // Counter with color transitions
#define ENABLE_BOUNCING_COUNTER    1   // Bouncing counter animation

// Internal macros for enable checks (don't modify)
#define ANIM_ENABLED_SIMPLE_COUNTER      ENABLE_SIMPLE_COUNTER
#define ANIM_ENABLED_RANDOM_POSITION     ENABLE_RANDOM_POSITION
#define ANIM_ENABLED_COLOR_TRANSITION    ENABLE_COLOR_TRANSITION
#define ANIM_ENABLED_BOUNCING_COUNTER    ENABLE_BOUNCING_COUNTER

// -----------------------------------------------------
// Animation Duration Configuration (milliseconds)
// -----------------------------------------------------
#define DURATION_SIMPLE_COUNTER      10000   // Simple counter animation (10 seconds)
#define DURATION_RANDOM_POSITION     10000   // Random position animation (10 seconds)
#define DURATION_COLOR_TRANSITION    15000   // Color transition animation (15 seconds)
#define DURATION_BOUNCING_COUNTER    60000   // Bouncing counter animation (30 seconds)

#endif // ANIMATION_CONFIG_H