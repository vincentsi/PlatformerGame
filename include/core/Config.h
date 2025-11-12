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
}
