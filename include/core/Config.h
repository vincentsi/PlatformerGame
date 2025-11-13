#pragma once

namespace Config {
    // Window settings
    constexpr unsigned int WINDOW_WIDTH = 1280;
    constexpr unsigned int WINDOW_HEIGHT = 720;
    constexpr const char* WINDOW_TITLE = "Platformer Game - MVP";
    constexpr unsigned int FRAMERATE_LIMIT = 60;

    // Game constants
    constexpr float GRAVITY = 980.0f;           // pixels/s²
    constexpr float JUMP_VELOCITY = -500.0f;    // pixels/s (negative = up)
    constexpr float MOVE_SPEED = 300.0f;        // pixels/s
    constexpr float FRICTION = 0.85f;
    constexpr float COYOTE_TIME = 0.1f;         // seconds
    constexpr float JUMP_BUFFER = 0.1f;         // seconds

    // Player settings
    constexpr float PLAYER_WIDTH = 32.0f;
    constexpr float PLAYER_HEIGHT = 48.0f;

    // Platform settings
    constexpr float PLATFORM_HEIGHT = 20.0f;

    // Camera settings
    constexpr float CAMERA_SMOOTHING = 0.1f;

    // Death settings
    constexpr float DEATH_ZONE_Y = 800.0f;      // Y position below which player dies (just below screen)
    constexpr float RESPAWN_TIME = 0.5f;        // seconds before respawn

    // Debug
    constexpr bool SHOW_FPS = true;
    constexpr bool SHOW_COLLISION_BOXES = false;

    // Special abilities
    constexpr float KINETIC_WAVE_RANGE = 150.0f;      // Lyra's kinetic wave range
    constexpr float KINETIC_WAVE_FORCE = 800.0f;      // Force of kinetic push
    constexpr float KINETIC_WAVE_COOLDOWN = 3.0f;     // Cooldown in seconds
    
    constexpr float HACK_RANGE = 100.0f;              // Noah's hack range
    constexpr float HACK_COOLDOWN = 2.0f;             // Cooldown in seconds
    
    constexpr float BERSERK_DURATION = 8.0f;          // Sera's berserk duration
    constexpr float BERSERK_COOLDOWN = 20.0f;         // Cooldown in seconds
    constexpr float BERSERK_HEAL_RATE = 0.5f;         // HP per second during berserk
    constexpr float BERSERK_SPEED_BOOST = 1.5f;       // Speed multiplier

    // Gameplay constants
    constexpr float MAX_DELTA_TIME = 0.1f;            // Cap delta time to avoid spiral of death
    constexpr float STOMP_TOLERANCE_BASE = 10.0f;     // Base tolerance for stomp detection
    constexpr float KINETIC_WAVE_DOT_THRESHOLD = 0.3f; // Dot product threshold for kinetic wave direction
    constexpr float ENEMY_BOUNCE_VELOCITY = -300.0f;   // Bounce velocity when stomping enemy
    constexpr float FLYING_ENEMY_BOUNCE_VELOCITY = -400.0f; // Higher bounce for flying enemy (secret)
    constexpr float SECRET_PLATFORM_SIZE = 2.0f;      // Size threshold for secret platform detection
}
