# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build and Run Commands

### Development Build (Windows)

```bash
# From PlatformerGame/ directory
cd build
cmake --build . --config Release

# Run game
cd bin/Release
./PlatformerGame.exe
```

### Quick Rebuild Workflow

```bash
# Kill running instances, rebuild, and launch
taskkill /F /IM PlatformerGame.exe 2>nul
cd build && cmake --build . --config Release
cd bin/Release && ./PlatformerGame.exe
```

### Clean Build

```bash
# Remove build directory and start fresh
rm -rf build
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

### Configure CMake with SFML

```bash
# If SFML_DIR is not in environment
cmake .. -DSFML_DIR=C:/SFML/lib/cmake/SFML

# With vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=[path_to_vcpkg]/scripts/buildsystems/vcpkg.cmake
```

## Architecture Overview

### Entity System (Inheritance Hierarchy)

```
Entity (abstract base)
├── Player
└── Enemy (abstract)
    └── PatrolEnemy
```

**Key Pattern**: All game entities inherit from Entity base class with virtual `update(dt)` and `draw(window)` methods. The Entity class is abstract - concrete entities must implement these methods.

### State Management

The game uses a state machine with `GameState` enum (TitleScreen, Playing, Paused, Settings, Controls, Transitioning). Critical pattern:

- `setState(newState)` updates both gameState AND previousState
- Direct `gameState = newState` assignment preserves previousState (used for sibling transitions like Settings ↔ Controls)
- Back buttons use `setState(previousState)` to return to parent menu

### Menu System (Observer Pattern)

All menus inherit from `Menu` base class and use callback functions:

```cpp
menu->setCallbacks(
    [this]() { /* action 1 */ },
    [this]() { /* action 2 */ }
);
```

Menus support both keyboard (via InputConfig) and mouse (hover + click) navigation.

### Level Loading Pipeline

1. `LevelLoader::loadFromFile()` parses JSON → creates `LevelData`
2. `Game::loadLevel()` moves ownership: `platforms = std::move(currentLevel->platforms)`
3. Enemies are currently hardcoded in loadLevel() - JSON integration is TODO

### Physics System

Two-phase collision resolution:

1. **Player update** - Apply gravity, input, velocity
2. **Collision resolution** - `CollisionSystem::resolveCollision()` checks all platforms, modifies position/velocity by reference
3. **Post-collision** - `player->setGrounded(grounded)` to enable jumping

Important: Collision resolution MUST happen before checking grounded state.

### Input Configuration (Singleton)

`InputConfig::getInstance()` manages customizable key bindings. Saved to `keybindings.cfg` as plain text (SFML key codes as integers).

Pattern for using configurable keys:

```cpp
const InputBindings& bindings = InputConfig::getInstance().getBindings();
if (sf::Keyboard::isKeyPressed(bindings.jump)) {
    player->jump();
}
```

### Save System

`SaveSystem` handles binary saves to `savegame.dat`:

- Uses `std::fstream` with binary mode
- Saves: currentLevel, totalDeaths, totalPlayTime, levelsUnlocked, levelBestTimes
- Save path determined by `SaveSystem::getSavePath()` (uses temp directory on Windows)

### Enemy-Player Collision (Stomp Mechanic)

Collision detection distinguishes between stomp (kill enemy) vs side hit (kill player):

```cpp
bool playerFalling = player->getVelocity().y > 0;
bool hitFromAbove = playerBounds.top + playerBounds.height <= enemyBounds.top + tolerance;

if (playerFalling && hitFromAbove) {
    enemy->kill();  // Stomp
    player->setVelocity(vx, -300.0f);  // Bounce
} else {
    player->die();  // Side/bottom collision
}
```

## Critical File Locations

### Configuration

- `include/core/Config.h` - Window size, physics constants, debug flags
- `include/physics/PhysicsConstants.h` - Player movement values (MOVE_SPEED, JUMP_VELOCITY, GRAVITY, etc.)

### Level Data

- `assets/levels/*.json` - Level definitions (platforms, checkpoints, goalZone, cameraZones)
- Format: See `LevelLoader.cpp` for JSON schema

### Persistent Data

- `keybindings.cfg` - Saved key bindings (one integer per line)
- `savegame.dat` - Binary save file (not human-readable)

## Adding New Content

### New Enemy Type

1. Create `include/entities/NewEnemy.h` inheriting from `Enemy`
2. Implement `src/entities/NewEnemy.cpp` with `update(dt)` logic
3. Add to `CMakeLists.txt` SOURCES and HEADERS
4. Include in `Game.h` and instantiate in `loadLevel()`

### New Menu

1. Create menu class inheriting from `Menu`
2. Add new `GameState` enum value in `GameState.h`
3. Create instance in `Game.h` private members
4. Initialize in `Game()` constructor with callbacks
5. Handle in `processEvents()`, `update()`, and `render()` switch cases

### New Level

1. Create `assets/levels/levelX.json` following existing schema
2. Update `loadLevel()` transition logic (currently hardcoded to level2, level3, etc.)

## Important Patterns and Conventions

### Memory Management

- Use `std::unique_ptr` for owned objects
- Use `std::vector<std::unique_ptr<T>>` for collections
- Raw pointers only for non-owning references (e.g., `AudioManager*` passed to SettingsMenu)

### Rendering Order

Entities are drawn in this order (back to front):

1. Platforms
2. Checkpoints
3. Goal zones
4. Enemies
5. Particles
6. Player
7. UI (fixed camera view)

### Event Flags Pattern (Player)

Player uses one-frame event flags for effects:

```cpp
if (player->hasJustJumped()) {
    // Emit particles, play sound
}
player->clearEventFlags();  // MUST call after processing
```

### ESC Key Handling

- ESC during Playing → Opens pause menu (handled in `Game::processEvents()`)
- ESC in menus → Handled by each menu's `handleInput()` (Back button or cancel action)
- Never handle ESC in TitleScreen (prevents accidental game close)

## Common Pitfalls

1. **File locking on Windows**: Use `taskkill /F /IM PlatformerGame.exe` before rebuilding
2. **Variable shadowing warning**: `playerBounds` declared twice in Game.cpp (harmless but noisy)
3. **Missing DLLs**: CMake copies SFML DLLs to bin/Release but may fail - manually copy from C:/SFML/bin if needed
4. **Dead enemies linger**: Enemies are not removed from vector when killed (cleanup TODO)
5. **Collision order matters**: Always resolve collisions BEFORE setting grounded state

## Current Implementation Status (Week 5)

**Completed:**

- Core gameplay (movement, jumping, coyote time, jump buffering)
- Full menu system (keyboard + mouse, settings, key rebinding)
- Save/load system
- Particle effects, camera shake, screen transitions
- Audio system (sounds + music with volume control)
- Base enemy system with PatrolEnemy
- Combat system (stomp to kill enemies)

**In Progress:**

- Additional enemy types (Stationary, Flying)
- Loading enemies from JSON levels

**TODO:**

- Power-ups and collectibles
- Multiple complete levels
- Boss enemies
- Game over screen

## Documentation

Technical documentation is available in `docs/technical/`:

- `menu-system.md` - Complete menu architecture and navigation
- `enemy-system.md` - Enemy class hierarchy and combat system
- `collision-system.md` - AABB collision resolution
- `physics-system.md` - Player physics and movement
- `camera-system.md` - Camera following and smoothing
- `death-respawn-system.md` - Death handling and respawn
- `ui-system.md` - HUD and interface elements

Refer to these when modifying respective systems.

🚨 **BEFORE editing any files, you MUST Read at least 3 files** that will help you understand how to make coherent and consistent changes.

This is **NON-NEGOTIABLE**. Do not skip this step under any circumstances. Reading existing files ensures:

- Code consistency with project patterns
- Proper understanding of conventions
- Following established architecture
- Avoiding breaking changes

**Types of files you MUST read:**

1. **Similar files**: Read files that do similar functionality to understand patterns and conventions
2. **Imported dependencies**: Read the definition/implementation of any imports you're not 100% sure how to use correctly - understand their API, types, and usage patterns
3. **Related middleware/services**: Understand how existing code handles similar use cases

**Steps to follow:**

1. Read at least 4 relevant existing files (similar functionality + imported dependencies)
2. Understand the patterns, conventions, and API usage
3. Only then proceed with creating/editing files

## Communication Rules

**CRITICAL - Follow these rules at all times:**

- NEVER suggest to "continue tomorrow" or stop working
- NEVER say you'll "do it faster" or promise speed improvements
- NEVER make decisions about when to stop or continue work
- The USER decides when to stop, continue, or change direction
- Focus ONLY on executing the current task as requested
- Wait for USER instructions before moving to next steps
