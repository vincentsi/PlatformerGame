# Tilesets Directory

This directory contains tileset textures for environments.

## Structure

```
tilesets/
├── zone1_laboratory.png    # Zone 1 tileset (32×32 tiles)
├── zone2_medical.png        # Zone 2 tileset
├── zone3_maintenance.png    # Zone 3 tileset
├── zone4_bio.png           # Zone 4 tileset
├── zone5_confinement.png   # Zone 5 tileset
└── zone6_central.png       # Zone 6 tileset
```

## Guidelines

- Tile size: 32×32 pixels
- Tileset dimensions: 256×256 (8×8 tiles) or 512×512 (16×16 tiles)
- Format: PNG with transparency
- Seamless tiling required for repeating tiles

## Creating Tilesets

1. Use Tiled Map Editor to design tilesets
2. Follow Zone color palettes from `plan/zone-visual-breakdown.md`
3. Test tiling in game to ensure seamless repetition

See `plan/asset-production-guide.md` for detailed workflow.

