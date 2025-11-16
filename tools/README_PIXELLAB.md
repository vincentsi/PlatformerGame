# Guide d'utilisation PixelLab

## Installation rapide

**Option 1: Installer Python**

1. Télécharge Python depuis https://www.python.org/downloads/
2. Coche "Add Python to PATH" lors de l'installation
3. Ouvre un nouveau terminal et tape:
   ```bash
   pip install requests
   ```

**Option 2: Utiliser MCP directement dans Cursor** (recommandé)

1. Ouvre Cursor Settings (Ctrl+,)
2. Cherche "MCP" ou "Model Context Protocol"
3. Ajoute le serveur PixelLab avec ton token

## Utilisation du script

Une fois Python installé:

```bash
# Depuis le dossier PlatformerGame/tools
cd PlatformerGame/tools

# Teste la connexion API
python pixellab_generator.py test

# Génère un personnage
python pixellab_generator.py character "Lyra, girl with cyan hair, lab suit, pixel art 48x64"

# Génère une animation
python pixellab_generator.py animate <character_id> walk
```

## Utilisation via MCP (recommandé)

Une fois MCP configuré dans Cursor, je peux directement:

- `create_character` - Créer Lyra
- `animate_character` - Ajouter walk/idle/run
- `create_sidescroller_tileset` - Tilesets de labo

Dis-moi simplement "Génère l'animation de marche de Lyra" et je le fais.

## Note importante

Les endpoints de l'API peuvent varier. Si le script ne marche pas, on ajustera après avoir testé.
