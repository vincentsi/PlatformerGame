# Sprites Directory

This directory contains all sprite assets for the game.

## Structure

```
sprites/
├── characters/        # Player characters (Lyra, Noah, Sera)
├── enemies/          # Enemy sprites
├── objects/          # Interactive objects
└── fx/              # Visual effects and particles
```

## Guidelines

- All sprites should be in PNG format with transparency
- Use power-of-2 dimensions for textures (e.g., 512×512, 1024×1024)
- Disable smoothing for pixel art (handled in SpriteManager)
- Follow naming convention: `entity_action.png` (e.g., `lyra_idle.png`)

## Next Steps

1. Download free assets from `plan/free-assets-resources.md`
2. Create placeholder sprites using tools from `plan/asset-production-guide.md`
3. Follow guidelines in `plan/visual-design-bible.md` for consistency

For detailed asset creation workflow, see `plan/asset-production-guide.md`.

