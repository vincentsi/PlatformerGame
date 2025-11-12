# Code Review - PlatformerGame

## 🔴 Problèmes Critiques (À corriger immédiatement)

### 1. **Mouvement Frame-Dependent (Game.cpp:212-215)**

**Problème**: Utilisation de `1.0f / Config::FRAMERATE_LIMIT` au lieu de `dt`

```cpp
// ❌ MAUVAIS
player->moveLeft(1.0f / Config::FRAMERATE_LIMIT);
```

**Impact**: Le mouvement sera incorrect si le FPS change (lag, vsync off, etc.)
**Solution**: Passer `dt` depuis `update()` à `handleInput()`

### 2. **Hardcoded Input dans Player (Player.cpp:87)**

**Problème**: Utilisation directe de `sf::Keyboard::Space` au lieu d'InputConfig

```cpp
// ❌ MAUVAIS
if (!sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
```

**Impact**: Ignore la configuration personnalisée du joueur
**Solution**: Passer InputConfig ou une callback

### 3. **Pas de Validation des Inputs (InputConfig.cpp:112-133)**

**Problème**: Aucune validation des valeurs lues depuis le fichier

```cpp
// ❌ MAUVAIS - Pas de vérification si key est valide
file >> key; bindings.moveLeft = static_cast<sf::Keyboard::Key>(key);
```

**Impact**: Crash si fichier corrompu ou valeurs invalides
**Solution**: Valider les valeurs avant conversion

### 4. **Binary Save Non-Portable (SaveSystem.cpp:18)**

**Problème**: `write(&data, sizeof(SaveData))` n'est pas portable

```cpp
// ❌ MAUVAIS - Endianness, padding, alignment
file.write(reinterpret_cast<const char*>(&data), sizeof(SaveData));
```

**Impact**: Sauvegarde ne fonctionnera pas entre différentes plateformes/compilateurs
**Solution**: Utiliser sérialisation JSON ou format texte

### 5. **Couleur Hardcodée dans Respawn (Player.cpp:252)**

**Problème**: Reset vers Green au lieu d'utiliser characterType

```cpp
// ❌ MAUVAIS
shape.setFillColor(sf::Color::Green); // Back to normal color
```

**Impact**: Tous les personnages deviennent verts après respawn
**Solution**: Utiliser la couleur du characterType

### 6. **Duplication de Code (Game.cpp:537-617)**

**Problème**: `loadLevel()` et `loadLevel(const string&)` ont 80% de code dupliqué
**Impact**: Maintenance difficile, bugs à corriger deux fois
**Solution**: Factoriser le code commun

### 7. **Pas de Gestion d'Erreur Level Loading (Game.cpp:539,565)**

**Problème**: Pas de vérification si `loadFromFile` retourne nullptr

```cpp
// ❌ MAUVAIS - Pas de fallback si échec
currentLevel = LevelLoader::loadFromFile("assets/levels/level1.json");
if (currentLevel) {
    // ...
}
// Que faire si currentLevel est null?
```

**Impact**: Crash silencieux ou comportement imprévisible
**Solution**: Ajouter fallback ou niveau par défaut

---

## 🟡 Problèmes Modérés (À améliorer)

### 8. **Constantes Dupliquées (Config.h vs PhysicsConstants.h)**

**Problème**: `GRAVITY` définie dans deux endroits

- `Config::GRAVITY = 980.0f`
- `Physics::GRAVITY = 980.0f`
  **Impact**: Incohérence possible, maintenance difficile
  **Solution**: Utiliser une seule source de vérité

### 9. **Manque de Const-Correctness**

**Problème**: Beaucoup de méthodes pourraient être `const`

```cpp
// ❌ Devrait être const
sf::Vector2f getPosition() const; // ✅ Déjà const
float getMoveSpeed() const; // ✅ Déjà const
void update(float dt); // ❌ Ne peut pas être const (modifie state)
```

**Impact**: Moins de sécurité de type, optimisation manquée
**Solution**: Marquer méthodes appropriées comme `const`

### 10. **Vérification Font Non-Robuste (Game.cpp:526)**

**Problème**: `getInfo().family != ""` pour vérifier si font est chargée

```cpp
// ❌ FRAGILE
if (Config::SHOW_FPS && debugFont.getInfo().family != "") {
```

**Impact**: Peut échouer avec certaines fonts
**Solution**: Utiliser flag booléen ou vérifier si font est valide autrement

### 11. **Pas de Gestion d'Erreur Audio (Game.cpp:32-37)**

**Problème**: Chargement audio sans vérification

```cpp
// ❌ Pas de vérification si fichier existe
audioManager->loadSound("jump", "assets/sounds/jump.wav");
```

**Impact**: Erreurs silencieuses si fichiers manquants
**Solution**: Vérifier retour de `loadSound()` et logger

### 12. **Collision System - Cas Limites**

**Problème**: Pas de gestion si rectangles complètement imbriqués

```cpp
// Dans CollisionSystem::resolveCollision
// Que faire si movingRect est complètement dans staticRect?
```

**Impact**: Comportement imprévisible dans cas extrêmes
**Solution**: Ajouter gestion des cas limites

### 13. **Memory Leak Potentiel - Ennemis Morts**

**Problème**: Ennemis tués restent dans le vector

```cpp
// ❌ Ennemis morts ne sont jamais supprimés
for (auto& enemy : enemies) {
    if (enemy && enemy->isAlive()) {
        // ...
    }
}
```

**Impact**: Fuite mémoire progressive
**Solution**: Supprimer ennemis morts du vector

### 14. **Pas de Bounds Checking (InputConfig.cpp:122-128)**

**Problème**: Pas de vérification si fichier a assez de lignes

```cpp
// ❌ Peut lire des valeurs invalides si fichier incomplet
file >> key; bindings.moveLeft = static_cast<sf::Keyboard::Key>(key);
```

**Impact**: Comportement imprévisible si fichier corrompu
**Solution**: Valider chaque lecture

### 15. **String vs Char Array (SaveSystem.h:11)**

**Problème**: Utilisation de `char[64]` au lieu de `std::string`

```cpp
// ❌ Ancien style C
char activeCheckpointId[64];
```

**Impact**: Buffer overflow possible, moins sûr
**Solution**: Utiliser `std::string` (nécessite changement format sauvegarde)

---

## 🟢 Améliorations Recommandées

### 16. **RAII et Smart Pointers**

✅ **Bien fait**: Utilisation de `std::unique_ptr` partout
⚠️ **Amélioration**: Vérifier que tous les raw pointers sont non-owning

### 17. **Encapsulation**

✅ **Bien fait**: Classes bien encapsulées
⚠️ **Amélioration**: Réduire l'accès direct aux membres internes

### 18. **Constexpr et Inline**

✅ **Bien fait**: `constexpr` pour constantes
⚠️ **Amélioration**: Plus de `constexpr` pour fonctions simples

### 19. **Error Handling**

⚠️ **Amélioration**: Utiliser `std::optional` ou `expected` pour erreurs
⚠️ **Amélioration**: Logger les erreurs au lieu de `std::cout`

### 20. **Performance**

✅ **Bien fait**: Delta time pour frame-independent
⚠️ **Amélioration**: Réservation de capacité pour vectors si taille connue
⚠️ **Amélioration**: Éviter allocations dans la boucle de jeu

---

## 📊 Score Global

**Qualité du Code**: 7/10

- ✅ Architecture solide
- ✅ Utilisation moderne de C++ (smart pointers, RAII)
- ✅ Bonne séparation des responsabilités
- ❌ Quelques bugs critiques (frame-dependent movement)
- ❌ Manque de gestion d'erreur robuste
- ❌ Duplication de code

**Recommandation**: Corriger les problèmes critiques avant d'ajouter de nouvelles features.

---

## 🔧 Corrections Prioritaires

1. **URGENT**: Corriger mouvement frame-dependent (Game.cpp:212-215)
2. **URGENT**: Corriger couleur respawn (Player.cpp:252)
3. **IMPORTANT**: Valider inputs fichiers (InputConfig.cpp)
4. **IMPORTANT**: Gérer erreurs level loading
5. **IMPORTANT**: Supprimer ennemis morts du vector
6. **MOYEN**: Factoriser code dupliqué loadLevel()
7. **MOYEN**: Améliorer const-correctness

---

## ✅ Corrections Appliquées (Date: Aujourd'hui)

### 1. Mouvement Frame-Independent ✅

- **Problème**: Paramètre `dt` inutile dans `moveLeft()`/`moveRight()`
- **Solution**: Supprimé le paramètre, la vélocité est déjà multipliée par `dt` dans `update()`
- **Fichiers**: `Player.h`, `Player.cpp`, `Game.cpp`

### 2. Couleur Respawn ✅

- **Problème**: Couleur hardcodée à Green après respawn
- **Solution**: Utilise maintenant `characterType` pour définir la couleur correcte
- **Fichiers**: `Player.cpp:respawn()`

### 3. Input Configuré ✅

- **Problème**: Utilisation directe de `sf::Keyboard::Space` dans Player
- **Solution**: Utilise maintenant `InputConfig::getInstance().getBindings().jump`
- **Fichiers**: `Player.cpp:update()`

### 4. Validation Inputs ✅

- **Problème**: Pas de validation des clés lues depuis le fichier
- **Solution**: Ajout validation avec `isValidKey()` qui vérifie si key est dans la plage valide
- **Fichiers**: `InputConfig.cpp:loadFromFile()`

### 5. Cleanup Ennemis Morts ✅

- **Problème**: Ennemis morts restaient dans le vector (fuite mémoire)
- **Solution**: Utilise `std::remove_if` + `erase` pour supprimer les ennemis morts après update
- **Fichiers**: `Game.cpp:update()`

---

## 📚 Ressources

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [Game Programming Patterns](https://gameprogrammingpatterns.com/)
- [SFML Best Practices](https://www.sfml-dev.org/tutorials/)
