# Documentation du Platformer Game

## Vue d'ensemble

Jeu de plateforme 2D développé en C++ avec SFML 2.6.1. Le projet suit une architecture orientée objet avec des systèmes modulaires pour la physique, les entités, l'audio, et l'interface utilisateur.

## Architecture du Projet

```
PlatformerGame/
├── include/          # Fichiers d'en-tête (.h)
│   ├── core/         # Systèmes centraux
│   ├── entities/     # Entités du jeu
│   ├── world/        # Éléments du monde
│   ├── physics/      # Système de physique
│   ├── ui/           # Interface utilisateur
│   ├── effects/      # Effets visuels et audio
│   └── audio/        # Gestion audio
├── src/              # Implémentations (.cpp)
│   ├── core/
│   ├── entities/
│   ├── world/
│   ├── physics/
│   ├── ui/
│   ├── effects/
│   └── audio/
├── assets/           # Ressources
│   ├── fonts/
│   ├── sounds/
│   ├── music/
│   └── levels/
└── build/            # Fichiers de compilation
```

---

## Systèmes Implémentés

### 1. Système Core

#### Game.h / Game.cpp
**Rôle:** Classe principale qui gère la boucle de jeu et orchestre tous les systèmes.

**Responsabilités:**
- Boucle de jeu principale (60 FPS)
- Gestion des états (TitleScreen, Playing, Paused, Settings, Controls, Transitioning)
- Orchestration des systèmes (physique, audio, particules, etc.)

**États du jeu:**
```cpp
enum class GameState {
    TitleScreen,    // Menu principal
    Playing,        // Jeu en cours
    Paused,         // Jeu en pause
    Settings,       // Menu paramètres
    Controls,       // Menu configuration touches
    Transitioning   // Transition entre niveaux
};
```

**Méthodes principales:**
- `run()` - Boucle de jeu principale
- `processEvents()` - Gestion des événements (clavier, souris)
- `update(float dt)` - Mise à jour de la logique (60 fois/sec)
- `render()` - Rendu graphique
- `loadLevel(path)` - Chargement d'un niveau depuis JSON

#### Config.h
**Rôle:** Constantes de configuration globales.

```cpp
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr int FRAMERATE_LIMIT = 60;
constexpr bool SHOW_FPS = true;
```

#### InputConfig.h / InputConfig.cpp
**Rôle:** Gestion des touches configurables (singleton).

**Fonctionnalités:**
- Configuration des touches pour mouvement, sauts, menus
- Sauvegarde/chargement depuis `keybindings.cfg`
- Noms des touches pour affichage
- Reset aux valeurs par défaut

**Touches par défaut:**
```cpp
moveLeft = A
moveRight = D
jump = Space
menuUp = W
menuDown = S
menuSelect = Enter
pause = Escape
```

**Utilisation:**
```cpp
InputConfig& config = InputConfig::getInstance();
config.setBinding("jump", sf::Keyboard::Space);
sf::Keyboard::Key jumpKey = config.getBinding("jump");
config.saveToFile(); // Sauvegarde dans keybindings.cfg
```

#### SaveSystem.h
**Rôle:** Système de sauvegarde/chargement de progression.

**Structure SaveData:**
```cpp
struct SaveData {
    int currentLevel;           // Niveau actuel
    int totalDeaths;            // Nombre total de morts
    float totalPlayTime;        // Temps de jeu total
    std::vector<bool> levelsUnlocked;
    std::vector<float> levelBestTimes;
};
```

**Fichier:** `savegame.dat` (format binaire)

---

### 2. Système d'Entités

#### Entity.h / Entity.cpp
**Rôle:** Classe de base abstraite pour toutes les entités.

**Membres protégés:**
```cpp
sf::Vector2f position;  // Position dans le monde
sf::Vector2f velocity;  // Vélocité (pixels/sec)
sf::Vector2f size;      // Dimensions
bool isGrounded;        // Contact avec le sol
```

**Méthodes virtuelles pures:**
- `virtual void update(float dt) = 0;`
- `virtual void draw(sf::RenderWindow&) = 0;`

#### Player.h / Player.cpp
**Rôle:** Le personnage jouable.

**Constantes (PhysicsConstants.h):**
```cpp
MOVE_SPEED = 300.0f           // Vitesse de déplacement
JUMP_VELOCITY = -500.0f       // Force du saut
GRAVITY = 1200.0f             // Gravité
MAX_FALL_SPEED = 600.0f       // Vitesse de chute max
JUMP_CUT_MULTIPLIER = 0.5f    // Multiplicateur si bouton relâché
COYOTE_TIME = 0.15f           // Temps de grâce après quitter sol
JUMP_BUFFER_TIME = 0.1f       // Temps pour buffer le saut
```

**Mécaniques:**
- **Mouvement horizontal** - A/D ou flèches
- **Saut** - Space avec hauteur variable (relâcher bouton = saut court)
- **Coyote Time** - Peut sauter 0.15s après avoir quitté une plateforme
- **Jump Buffering** - Peut appuyer sur saut 0.1s avant d'atterrir
- **Mort et respawn** - Timer de 1 seconde, respawn au dernier checkpoint

**Système de mort:**
```cpp
void die();           // Tue le joueur
void respawn();       // Respawn après 1 seconde
bool isDead();        // Vérifie si mort
void setSpawnPoint(); // Définit le point de respawn
```

**Événements pour effets:**
```cpp
bool hasJustJumped();  // Vrai pendant 1 frame après saut
bool hasJustLanded();  // Vrai pendant 1 frame après atterrissage
void clearEventFlags(); // Nettoie les flags
```

#### Enemy.h / Enemy.cpp
**Rôle:** Classe de base pour tous les ennemis.

**Membres protégés:**
```cpp
sf::RectangleShape shape;      // Forme visuelle
EnemyType type;                // Type d'ennemi
bool alive;                    // Vivant/mort
float speed;                   // Vitesse de déplacement
float patrolLeftBound;         // Limite gauche de patrouille
float patrolRightBound;        // Limite droite de patrouille
```

**Types d'ennemis:**
```cpp
enum class EnemyType {
    Patrol,      // Patrouille gauche-droite
    Stationary,  // Immobile (non implémenté)
    Flying       // Volant (non implémenté)
};
```

**Méthodes:**
```cpp
void kill();                                    // Tue l'ennemi
void setPatrolBounds(float left, float right); // Définit zone de patrouille
bool isAlive();                                // Vérifie si vivant
```

#### PatrolEnemy.h / PatrolEnemy.cpp
**Rôle:** Ennemi qui patrouille entre deux points.

**Comportement:**
- Se déplace de gauche à droite
- Inverse direction aux limites de patrouille
- Vitesse: 80 pixels/seconde
- Distance de patrouille par défaut: 150 pixels de chaque côté

**Constructeur:**
```cpp
PatrolEnemy(float x, float y, float patrolDistance = 150.0f);
```

**Logique update():**
```cpp
void update(float dt) {
    if (direction == Right) {
        position.x += speed * dt;
        if (position.x >= patrolRightBound) {
            direction = Left;
        }
    } else {
        position.x -= speed * dt;
        if (position.x <= patrolLeftBound) {
            direction = Right;
        }
    }
}
```

---

### 3. Système de Monde

#### Platform.h / Platform.cpp
**Rôle:** Plateformes solides.

**Types:**
```cpp
enum class PlatformType {
    Normal,    // Plateforme standard
    Moving,    // Plateforme mobile (non implémenté)
    Breakable  // Plateforme cassable (non implémenté)
};
```

**Couleurs:**
- Normal: Gris (128, 128, 128)

#### GoalZone.h / GoalZone.cpp
**Rôle:** Zone de fin de niveau.

**Fonctionnalités:**
- Animation de pulsation (scale entre 1.0 et 1.1)
- Couleur: Jaune (255, 215, 0)
- Détection collision avec joueur
- Émission de particules

#### Checkpoint.h / Checkpoint.cpp
**Rôle:** Points de sauvegarde dans le niveau.

**États:**
```cpp
enum class CheckpointState {
    Inactive,   // Gris (150, 150, 150)
    Activating, // Animation (0.3s)
    Active      // Vert (100, 255, 100)
};
```

**Fonctionnalités:**
- Détection collision avec joueur
- Animation d'activation (scale 1.0 → 1.5 → 1.2)
- Définit le point de respawn du joueur
- ID unique pour sauvegarde

#### Camera.h / Camera.cpp
**Rôle:** Caméra qui suit le joueur avec limites.

**Paramètres:**
```cpp
CAMERA_SMOOTH_FACTOR = 5.0f; // Vitesse de suivi
```

**Fonctionnalités:**
- Suit le joueur avec lerp (smooth)
- Respecte les limites min/max du niveau
- Support pour camera shake
- Centrage sur le joueur

**Utilisation:**
```cpp
camera->update(playerPosition, dt);
camera->setShakeOffset(shakeOffset);
camera->apply(window); // Applique la vue au rendu
```

#### LevelLoader.h / LevelLoader.cpp
**Rôle:** Chargement des niveaux depuis JSON.

**Format JSON:**
```json
{
    "name": "Level 1",
    "startPosition": { "x": 100, "y": 500 },
    "platforms": [
        {
            "x": 0, "y": 600,
            "width": 1280, "height": 40,
            "type": "normal"
        }
    ],
    "checkpoints": [
        {
            "id": "cp1",
            "x": 500, "y": 550,
            "width": 40, "height": 40
        }
    ],
    "goalZone": {
        "x": 1200, "y": 520,
        "width": 60, "height": 60
    },
    "cameraZones": [
        {
            "minX": 0, "maxX": 1280,
            "minY": 0, "maxY": 720
        }
    ]
}
```

**Utilisation:**
```cpp
std::unique_ptr<LevelData> level =
    LevelLoader::loadFromFile("assets/levels/level1.json");
```

---

### 4. Système de Physique

#### CollisionSystem.h
**Rôle:** Détection et résolution de collisions AABB.

**Méthode principale:**
```cpp
static bool resolveCollision(
    sf::FloatRect& movingRect,    // Rectangle en mouvement (modifié)
    sf::Vector2f& velocity,        // Vélocité (modifiée)
    const sf::FloatRect& staticRect, // Rectangle statique
    bool& grounded                 // Est au sol (modifié)
);
```

**Algorithme:**
1. Détecte intersection AABB
2. Calcule profondeur de pénétration pour chaque axe
3. Résout sur l'axe avec la plus petite pénétration
4. Annule la vélocité sur cet axe
5. Met à jour le flag `grounded` si collision par le bas

#### PhysicsConstants.h
**Rôle:** Constantes physiques du jeu.

```cpp
// Joueur
constexpr float PLAYER_MOVE_SPEED = 300.0f;
constexpr float PLAYER_JUMP_VELOCITY = -500.0f;
constexpr float GRAVITY = 1200.0f;
constexpr float MAX_FALL_SPEED = 600.0f;
constexpr float JUMP_CUT_MULTIPLIER = 0.5f;
constexpr float COYOTE_TIME = 0.15f;
constexpr float JUMP_BUFFER_TIME = 0.1f;
constexpr float RESPAWN_DELAY = 1.0f;
```

---

### 5. Système d'Interface

#### Menu.h / Menu.cpp
**Rôle:** Classe de base pour tous les menus.

**Fonctionnalités:**
- Navigation au clavier (haut/bas, entrée)
- Navigation à la souris (hover + clic)
- Callbacks pour actions
- Titre et items de menu
- Support touches configurables

**Utilisation:**
```cpp
Menu menu;
menu.setTitle("MAIN MENU");
menu.addItem("Start", []() { /* action */ });
menu.addItem("Quit", []() { /* action */ });
menu.handleInput(event);
menu.draw(window);
```

#### TitleScreen.h / TitleScreen.cpp
**Rôle:** Menu principal du jeu.

**Options:**
- New Game
- Continue (si sauvegarde existe)
- Settings
- Quit

#### PauseMenu.h / PauseMenu.cpp
**Rôle:** Menu pause (ESC pendant jeu).

**Options:**
- Resume (ESC pour reprendre)
- Settings
- Main Menu
- Quit

**Particularité:**
- ESC ouvre/ferme le menu
- Affiche le jeu en arrière-plan

#### SettingsMenu.h / SettingsMenu.cpp
**Rôle:** Menu de configuration audio.

**Paramètres:**
- Master Volume (0-100)
- Sound Volume (0-100)
- Music Volume (0-100)

**Contrôles:**
- Flèches gauche/droite: ajuster par pas de 5
- Navigation haut/bas entre paramètres
- ESC pour retourner

**Intégration:**
```cpp
SettingsMenu menu(audioManager);
menu.setCallbacks(
    []() { /* Aller à Controls */ },
    []() { /* Retour */ }
);
```

#### KeyBindingMenu.h / KeyBindingMenu.cpp
**Rôle:** Menu de configuration des touches.

**Touches configurables:**
- Move Left (défaut: A)
- Move Right (défaut: D)
- Jump (défaut: Space)
- Menu Up (défaut: W)
- Menu Down (défaut: S)
- Menu Select (défaut: Enter)

**Note:** Pause (ESC) n'est pas configurable.

**Processus de rebind:**
1. Sélectionner une action
2. Appuyer Entrée
3. "< Press Key >" s'affiche en jaune
4. Appuyer nouvelle touche
5. ESC pour annuler

**Layout:**
- Labels alignés à gauche
- Valeurs alignées au milieu
- Boutons en bas (Reset to Defaults, Back)

#### GameUI.h / GameUI.cpp
**Rôle:** Interface pendant le jeu (HUD).

**Affichage:**
- Timer (temps écoulé)
- Compteur de morts
- Message de victoire (niveau terminé)

**Position:**
- Timer: haut gauche (10, 10)
- Morts: haut gauche (10, 40)
- Victoire: centre écran

---

### 6. Système d'Effets

#### ParticleSystem.h / ParticleSystem.cpp
**Rôle:** Système de particules pour effets visuels.

**Types d'effets:**
```cpp
emitJump(position);          // Particules blanches vers le bas
emitLanding(position);       // Particules blanches spread
emitDeath(position);         // Particules rouges explosion
emitVictory(position);       // Particules dorées ascendantes
emitGoalGlow(pos, size);     // Particules dorées dans zone
```

**Propriétés particule:**
```cpp
struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    sf::Color color;
    float lifetime;      // Durée de vie
    float maxLifetime;
};
```

**Caractéristiques:**
- Fade out progressif (alpha diminue)
- Gravité pour certains effets
- Nettoyage automatique des particules mortes
- Optimisé (vector avec reserve)

#### CameraShake.h / CameraShake.cpp
**Rôle:** Effet de tremblement de caméra.

**Intensités:**
```cpp
shakeLight();    // Intensité 5, durée 0.15s
shakeMedium();   // Intensité 10, durée 0.25s
shakeStrong();   // Intensité 20, durée 0.4s
```

**Algorithme:**
- Offset aléatoire basé sur intensité
- Décroissance linéaire jusqu'à 0
- Retourne sf::Vector2f(offsetX, offsetY)

**Utilisation:**
```cpp
cameraShake->shakeMedium();
cameraShake->update(dt);
sf::Vector2f offset = cameraShake->getOffset();
camera->setShakeOffset(offset);
```

#### ScreenTransition.h / ScreenTransition.cpp
**Rôle:** Transitions fade in/out entre niveaux.

**États:**
```cpp
enum class TransitionState {
    None,
    FadingOut,  // Écran devient noir
    FadingIn,   // Écran redevient transparent
    Complete
};
```

**Utilisation:**
```cpp
transition->startFadeOut(0.5f);  // Durée 0.5s
transition->update(dt);

if (transition->isFadedOut()) {
    loadNextLevel();
    transition->startFadeIn(0.5f);
}

if (transition->isComplete()) {
    resumeGameplay();
}
```

---

### 7. Système Audio

#### AudioManager.h / AudioManager.cpp
**Rôle:** Gestion centralisée de l'audio.

**Fonctionnalités:**
- Chargement sons/musiques depuis fichiers
- Contrôle volume global et par catégorie
- Support multiples sons simultanés
- Gestion automatique si fichiers manquants

**Volumes:**
```cpp
setMasterVolume(0-100);  // Volume global
setSoundVolume(0-100);   // Volume effets sonores
setMusicVolume(0-100);   // Volume musique
```

**Utilisation:**
```cpp
audioManager->loadSound("jump", "assets/sounds/jump.wav");
audioManager->loadMusic("gameplay", "assets/music/gameplay.ogg");

audioManager->playSound("jump", 80.0f);    // Volume 80%
audioManager->playMusic("gameplay", true); // Loop
audioManager->stopMusic();
```

**Sons du jeu:**
- jump.wav - Saut du joueur
- land.wav - Atterrissage
- death.wav - Mort
- victory.wav - Victoire
- checkpoint.wav - Activation checkpoint

---

## Flux de Jeu

### Démarrage
1. **Initialisation** (Game::Game())
   - Création window SFML
   - Chargement systèmes (audio, particules, etc.)
   - Création menus
   - Chargement configuration touches
   - Vérification sauvegarde existante

2. **Menu Principal** (GameState::TitleScreen)
   - New Game → loadLevel("level1.json")
   - Continue → charge SaveData et loadLevel()
   - Settings → GameState::Settings
   - Quit → fermeture

### Jeu en Cours
1. **Chargement Niveau**
   ```cpp
   loadLevel("assets/levels/levelX.json"):
   - Parse JSON
   - Crée plateformes
   - Crée checkpoints
   - Crée goal zone
   - Crée ennemis (actuellement 1 test)
   - Positionne joueur au startPosition
   - Configure limites caméra
   ```

2. **Boucle de Jeu** (60 FPS)
   ```cpp
   processEvents():
   - ESC → ouvre PauseMenu
   - Souris → hover/clic sur menus
   - Clavier → déplacement selon InputConfig

   update(dt):
   - Player.update()
   - Collisions plateformes
   - Événements joueur (jump, land)
   - Ennemis.update()
   - Collision joueur-ennemi → player.die()
   - Checkpoints
   - GoalZone → transition niveau suivant
   - Particules, shake, transitions

   render():
   - Clear sky blue
   - Apply camera
   - Draw plateformes
   - Draw checkpoints
   - Draw goal zone
   - Draw ennemis
   - Draw particules
   - Draw joueur
   - Reset view
   - Draw UI (timer, morts)
   - Draw transition overlay
   ```

3. **Mort du Joueur**
   - player.die() appelé
   - Particules de mort émises
   - Son "death" joué
   - Camera shake moyen
   - Timer 1 seconde
   - Respawn au dernier checkpoint (ou start)
   - Compteur morts incrémenté

4. **Fin de Niveau**
   - Joueur entre dans GoalZone
   - levelCompleted = true
   - Particules victoire + son
   - startFadeOut(0.5s)
   - Chargement niveau suivant
   - startFadeIn(0.5s)
   - Retour au jeu

### Pause
- ESC pendant jeu → GameState::Paused
- Jeu affiché en arrière-plan (figé)
- Menu pause par-dessus
- Resume → GameState::Playing
- Settings → GameState::Settings (previousState = Paused)
- Main Menu → GameState::TitleScreen (perd progression)
- Quit → fermeture

### Menus Imbriqués
```
TitleScreen
    ↓ Settings
    Settings
        ↓ Controls
        KeyBindingMenu
            ↓ Back
        Settings
            ↓ Back
    TitleScreen

Playing
    ↓ ESC
    PauseMenu
        ↓ Settings
        Settings
            ↓ Controls
            KeyBindingMenu
                ↓ Back
            Settings
                ↓ Back
        PauseMenu
            ↓ Resume / ESC
    Playing
```

**Gestion previousState:**
- `setState()` met à jour previousState
- Transitions directes (Settings ↔ Controls) utilisent `gameState =` pour préserver previousState
- Back utilise `setState(previousState)`

---

## État Actuel du Projet

### ✅ Complètement Implémenté
- Physique de base (gravité, collisions, saut)
- Système de niveau JSON
- Sauvegardes
- Menus complets (navigation clavier + souris)
- Configuration touches personnalisables
- Effets visuels (particules, shake, transitions)
- Audio (sons, musique, volumes)
- Checkpoints et respawn
- Timer et compteur morts
- Système d'ennemis de base (classe Enemy, PatrolEnemy)

### ⚠️ Partiellement Implémenté
- **Ennemis:**
  - ✅ Classe Enemy de base
  - ✅ PatrolEnemy fonctionnel
  - ✅ Collision joueur → ennemi (mort joueur)
  - ❌ Collision joueur → ennemi (tuer ennemi en sautant dessus)
  - ❌ Autres types (stationnaire, volant)
  - ❌ Chargement depuis niveau JSON
  - ❌ Système de PV pour ennemis

### ❌ Non Implémenté
- Système de combat/attaque joueur
- Power-ups et collectibles
- Score
- Multiples niveaux
- Menu sélection niveau
- Boss
- Écran game over
- Plus de types de plateformes (mobiles, cassables)

---

## Prochaines Étapes (Roadmap Semaine 5)

1. **Système de combat** (prioritaire)
   - Détecter si joueur saute sur tête ennemi
   - Tuer ennemi si attaque par dessus
   - Rebond du joueur après tuer ennemi
   - Effets visuels et sonores

2. **Autres types d'ennemis**
   - StationaryEnemy (immobile)
   - FlyingEnemy (vole en pattern)

3. **Intégration ennemis dans JSON**
   - Ajouter section "enemies" au format niveau
   - Charger depuis LevelLoader
   - Positionner selon niveau design

4. **Polish ennemis**
   - Animations (optionnel)
   - Système de PV (optionnel)
   - Drops (optionnel)

---

## Guide de Développement

### Ajouter un Nouveau Type d'Ennemi

1. **Créer les fichiers:**
   ```cpp
   // include/entities/NewEnemy.h
   #pragma once
   #include "entities/Enemy.h"

   class NewEnemy : public Enemy {
   public:
       NewEnemy(float x, float y);
       void update(float dt) override;

   private:
       // Variables spécifiques
   };
   ```

2. **Implémenter:**
   ```cpp
   // src/entities/NewEnemy.cpp
   #include "entities/NewEnemy.h"

   NewEnemy::NewEnemy(float x, float y)
       : Enemy(x, y, EnemyType::YourType)
   {
       speed = 100.0f;
       shape.setFillColor(sf::Color::Blue);
   }

   void NewEnemy::update(float dt) {
       if (!alive) return;
       // Logique de mouvement
       shape.setPosition(position);
   }
   ```

3. **Ajouter au CMakeLists.txt:**
   ```cmake
   set(SOURCES
       ...
       src/entities/NewEnemy.cpp
   )
   set(HEADERS
       ...
       include/entities/NewEnemy.h
   )
   ```

4. **Utiliser dans Game.cpp:**
   ```cpp
   #include "entities/NewEnemy.h"

   enemies.push_back(std::make_unique<NewEnemy>(x, y));
   ```

### Ajouter un Nouveau Menu

1. **Créer classe dérivée de Menu:**
   ```cpp
   // include/ui/MyMenu.h
   #pragma once
   #include "ui/Menu.h"

   class MyMenu : public Menu {
   public:
       MyMenu();
       void setCallbacks(std::function<void()> onAction);
   };
   ```

2. **Ajouter état dans GameState.h:**
   ```cpp
   enum class GameState {
       // ...
       MyMenu
   };
   ```

3. **Créer instance dans Game.h:**
   ```cpp
   std::unique_ptr<MyMenu> myMenu;
   ```

4. **Initialiser et gérer dans Game.cpp:**
   ```cpp
   myMenu = std::make_unique<MyMenu>();
   myMenu->setCallbacks([this]() { /* action */ });

   // Dans update() et render():
   else if (gameState == GameState::MyMenu && myMenu) {
       myMenu->update(dt);
       myMenu->draw(window);
   }
   ```

### Ajouter un Nouveau Niveau

1. **Créer fichier JSON:**
   ```json
   // assets/levels/level2.json
   {
       "name": "Level 2",
       "startPosition": { "x": 100, "y": 500 },
       "platforms": [
           // Vos plateformes
       ],
       "checkpoints": [
           // Vos checkpoints
       ],
       "goalZone": {
           "x": 1200, "y": 520,
           "width": 60, "height": 60
       },
       "cameraZones": [
           {
               "minX": 0, "maxX": 1280,
               "minY": 0, "maxY": 720
           }
       ]
   }
   ```

2. **Tester:**
   ```cpp
   // Dans Game.cpp, modifier temporairement:
   loadLevel("assets/levels/level2.json");
   ```

---

## Conseils et Best Practices

### Performance
- Utiliser `reserve()` sur vectors quand taille connue
- Éviter allocations dans boucle de jeu
- Nettoyer particules/entités mortes
- Limiter nombre particules simultanées

### Architecture
- Garder Entity comme classe de base abstraite
- Nouveaux systèmes = nouvelles classes
- Singleton pour systèmes globaux (InputConfig, SaveSystem)
- unique_ptr pour ownership, raw pointers pour références

### Collisions
- AABB pour tout actuellement
- Résoudre par axe minimal
- Toujours résoudre collisions AVANT mettre à jour flags (grounded, etc.)

### Audio
- Toujours vérifier si fichier chargé avant jouer
- Volume relatif (0-100) plus intuitif que (0-1)
- Musique en loop, sons en one-shot

### UI
- Menus héritent de Menu pour cohérence
- Supports clavier ET souris
- ESC pour retour dans sous-menus
- Centrer textes avec getLocalBounds()

---

## Fichiers de Configuration

### keybindings.cfg
Format texte, une touche par ligne (code SFML int):
```
0   # moveLeft (sf::Keyboard::A)
3   # moveRight (sf::Keyboard::D)
...
```

### savegame.dat
Format binaire (SaveSystem):
```cpp
- int currentLevel
- int totalDeaths
- float totalPlayTime
- vector<bool> levelsUnlocked
- vector<float> levelBestTimes
```

---

## Dépendances

- **SFML 2.6.1** - Graphics, Window, System, Audio
- **C++17** - Standard library, filesystem
- **CMake 3.16+** - Build system
- **nlohmann/json** - Parsing JSON (single header)

---

## Notes Techniques

### Coordonnées
- Origine (0,0) en haut à gauche
- X positif vers droite
- Y positif vers bas
- Unités en pixels

### Timing
- dt = delta time en secondes
- 60 FPS = dt ≈ 0.0166s
- Tous les mouvements * dt pour frame-independence

### Rendering Order
1. Background (sky blue)
2. Platforms
3. Checkpoints
4. Goal zone
5. Enemies
6. Particles
7. Player
8. UI (deaths, timer)
9. Transition overlay

### Memory Management
- Smart pointers partout (unique_ptr)
- RAII pour ressources SFML
- Pas de new/delete manuel
- Vectors pour collections dynamiques
