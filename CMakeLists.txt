cmake_minimum_required(VERSION 3.15)
project(PlatformerGame VERSION 1.0.0 LANGUAGES CXX)

# C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Source files
set(SOURCES
    src/core/main.cpp
    src/core/Game.cpp
    src/entities/Entity.cpp
    src/entities/Player.cpp
    src/entities/Enemy.cpp
    src/entities/PatrolEnemy.cpp
    src/entities/Spike.cpp
    src/entities/FlyingEnemy.cpp
    src/world/Platform.cpp
    src/world/Camera.cpp
    src/world/GoalZone.cpp
    src/world/Checkpoint.cpp
    src/world/InteractiveObject.cpp
    src/world/LevelLoader.cpp
    src/physics/CollisionSystem.cpp
    src/ui/GameUI.cpp
    src/ui/Menu.cpp
    src/ui/TitleScreen.cpp
    src/ui/PauseMenu.cpp
    src/ui/SettingsMenu.cpp
    src/ui/KeyBindingMenu.cpp
    src/core/SaveSystem.cpp
    src/core/InputConfig.cpp
    src/core/Logger.cpp
    src/effects/ParticleSystem.cpp
    src/effects/CameraShake.cpp
    src/effects/ScreenTransition.cpp
    src/audio/AudioManager.cpp
)

# Header files
set(HEADERS
    include/core/Game.h
    include/core/Config.h
    include/core/GameState.h
    include/core/SaveSystem.h
    include/core/Logger.h
    include/entities/Entity.h
    include/entities/Player.h
    include/entities/Enemy.h
    include/entities/PatrolEnemy.h
    include/entities/Spike.h
    include/entities/FlyingEnemy.h
    include/world/Platform.h
    include/world/Camera.h
    include/world/GoalZone.h
    include/world/Checkpoint.h
    include/world/InteractiveObject.h
    include/world/LevelLoader.h
    include/physics/CollisionSystem.h
    include/physics/PhysicsConstants.h
    include/ui/GameUI.h
    include/ui/Menu.h
    include/ui/TitleScreen.h
    include/ui/PauseMenu.h
    include/ui/SettingsMenu.h
    include/ui/KeyBindingMenu.h
    include/core/InputConfig.h
    include/effects/Particle.h
    include/effects/ParticleSystem.h
    include/effects/CameraShake.h
    include/effects/ScreenTransition.h
    include/audio/AudioManager.h
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE include)

# Find SFML
find_package(SFML 2.5 COMPONENTS graphics window system audio REQUIRED)

# Link SFML
target_link_libraries(${PROJECT_NAME}
    sfml-graphics
    sfml-window
    sfml-system
    sfml-audio
)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)

# Windows specific: Copy SFML DLLs
if(WIN32)
    # Copy SFML DLLs to executable directory
    foreach(LIB graphics window system audio)
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:sfml-${LIB}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        )
    endforeach()
endif()

# Enable warnings
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4)
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Print configuration
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "SFML version: ${SFML_VERSION}")
