# Platformer Game - MVP

Un jeu de plateforme 2D développé en C++ avec SFML.

## Prérequis

- **CMake** 3.15 ou plus récent
- **C++17** compatible compiler (MSVC, GCC, Clang)
- **SFML 2.5+** (graphics, window, system, audio)

---

## Installation SFML sur Windows

### Méthode 1 : Téléchargement manuel (Recommandé)

1. Télécharger SFML depuis le site officiel :
   - [https://www.sfml-dev.org/download.php](https://www.sfml-dev.org/download.php)
   - Choisir la version **SFML 2.6.1** pour **Visual Studio 2022 (64-bit)** ou votre compilateur

2. Extraire l'archive dans `C:\SFML` (ou un autre dossier de votre choix)

3. Ajouter SFML au PATH système :
   - Ouvrir "Modifier les variables d'environnement système"
   - Variables d'environnement → Variable système `Path` → Modifier
   - Ajouter : `C:\SFML\bin`

4. Créer une variable `SFML_DIR` :
   - Variables d'environnement → Nouvelle variable système
   - Nom : `SFML_DIR`
   - Valeur : `C:\SFML` (ou votre chemin d'installation)

### Méthode 2 : vcpkg (Plus simple mais plus long)

```bash
# Installer vcpkg
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
bootstrap-vcpkg.bat

# Installer SFML
vcpkg install sfml:x64-windows

# Intégrer vcpkg avec CMake
vcpkg integrate install
```

---

## Compilation du projet

### Windows (Visual Studio)

```bash
# Dans le dossier PlatformerGame/
mkdir build
cd build

# Avec SFML_DIR configuré
cmake ..

# OU avec vcpkg
cmake .. -DCMAKE_TOOLCHAIN_FILE=[chemin_vcpkg]/scripts/buildsystems/vcpkg.cmake

# Compiler
cmake --build . --config Release

# Lancer le jeu
cd bin\Release
PlatformerGame.exe
```

### Windows (MinGW)

```bash
mkdir build
cd build
cmake .. -G "MinGW Makefiles"
mingw32-make
cd bin
PlatformerGame.exe
```

### Linux

```bash
# Installer SFML
sudo apt-get install libsfml-dev

# Compiler
mkdir build
cd build
cmake ..
make

# Lancer
./bin/PlatformerGame
```

### macOS

```bash
# Installer SFML
brew install sfml

# Compiler
mkdir build
cd build
cmake ..
make

# Lancer
./bin/PlatformerGame
```

---

## Contrôles

| Action | Touches |
|--------|---------|
| Déplacer à gauche | `A` ou `←` |
| Déplacer à droite | `D` ou `→` |
| Sauter | `Espace` |
| Quitter | `Échap` |

---

## Architecture du projet

```
PlatformerGame/
├── CMakeLists.txt          # Configuration CMake
├── README.md               # Ce fichier
│
├── include/                # Headers (.h)
│   ├── core/
│   │   ├── Game.h
│   │   └── Config.h
│   ├── entities/
│   │   ├── Entity.h
│   │   └── Player.h
│   ├── world/
│   │   ├── Platform.h
│   │   └── Camera.h
│   └── physics/
│       ├── CollisionSystem.h
│       └── PhysicsConstants.h
│
├── src/                    # Source files (.cpp)
│   ├── core/
│   │   ├── main.cpp
│   │   └── Game.cpp
│   ├── entities/
│   │   ├── Entity.cpp
│   │   └── Player.cpp
│   ├── world/
│   │   ├── Platform.cpp
│   │   └── Camera.cpp
│   └── physics/
│       └── CollisionSystem.cpp
│
└── assets/                 # Ressources (textures, sons, fonts)
    ├── textures/
    ├── sounds/
    ├── fonts/
    └── levels/
```

---

## Features implémentées (MVP)

- ✅ Mouvement du joueur (gauche/droite)
- ✅ Saut avec gravité réaliste
- ✅ Collisions AABB avec plateformes
- ✅ Coyote time (grace period pour sauter après avoir quitté une plateforme)
- ✅ Jump buffering (input anticipé avant d'atterrir)
- ✅ Variable jump height (maintenir espace = sauter plus haut)
- ✅ Camera qui suit le joueur
- ✅ Niveau hardcodé pour test
- ✅ FPS counter (optionnel)
- ✅ 60 FPS stable

---

## Prochaines étapes (Roadmap)

Voir [../plan/roadmap.md](../plan/roadmap.md) pour la roadmap complète.

**Semaine 1-4 : MVP**
- [x] Setup projet
- [x] Mouvement de base
- [x] Saut et gravité
- [x] Collisions
- [ ] Polish du game feel
- [ ] Niveau complet jouable

**Phase 2+ :**
- Animations de sprites
- Tilemap system
- Ennemis
- Collectibles
- Audio
- Menus

---

## Troubleshooting

### Erreur "SFML not found"

**Solution** : Vérifier que `SFML_DIR` est bien configuré ou utiliser vcpkg.

```bash
cmake .. -DSFML_DIR=C:/SFML/lib/cmake/SFML
```

### Erreur "DLL not found" au lancement

**Solution** : Les DLLs SFML doivent être à côté de l'exécutable.

CMake copie automatiquement les DLLs, mais si ça ne fonctionne pas :
- Copier manuellement les fichiers `.dll` depuis `C:\SFML\bin` vers `build\bin\Release\`

### Le jeu lag / FPS instable

**Solution** :
- Compiler en mode Release : `cmake --build . --config Release`
- Vérifier que VSync est activé (déjà fait dans le code)

### Pas de font pour le FPS counter

**Solution** :
- Le jeu fonctionne sans font, le FPS ne s'affichera juste pas
- Pour afficher le FPS, placer une font `arial.ttf` dans `assets/fonts/`

---

## Configuration

Modifier les constantes dans [include/core/Config.h](include/core/Config.h) :

```cpp
// Fenêtre
constexpr unsigned int WINDOW_WIDTH = 1280;
constexpr unsigned int WINDOW_HEIGHT = 720;
constexpr unsigned int FRAMERATE_LIMIT = 60;

// Physique
constexpr float GRAVITY = 980.0f;
constexpr float JUMP_VELOCITY = -500.0f;
constexpr float MOVE_SPEED = 300.0f;

// Debug
constexpr bool SHOW_FPS = true;
constexpr bool SHOW_COLLISION_BOXES = false;
```

---

## License

Ce projet est à des fins d'apprentissage.

---

## Credits

- **SFML** : https://www.sfml-dev.org/
- **CMake** : https://cmake.org/

---

## Support

Pour toute question ou problème, ouvrir une issue sur le repository.

Bon jeu ! 🎮
