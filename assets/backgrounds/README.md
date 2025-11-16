# Backgrounds Directory

This directory contains parallax background layers for each zone.

## Structure

```
backgrounds/
├── zone1_bg_layer1.png    # Zone 1 far background
├── zone1_bg_layer2.png    # Zone 1 mid background
├── zone1_bg_layer3.png    # Zone 1 near background
└── ...                    # Repeat for zones 2-6
```

## Guidelines

- Resolution: 1920×1080 (full screen)
- Format: PNG
- 2-3 layers per zone (far, mid, near)
- Scroll speeds set in code:
  - Layer 1 (far): 0.2x camera speed
  - Layer 2 (mid): 0.5x camera speed
  - Layer 3 (near): 0.8x camera speed

## Creating Parallax Backgrounds

1. Design far background (abstract, dark)
2. Add mid layer with more detail
3. Add near layer with foreground elements
4. Test scrolling effect in game

See `plan/zone-visual-breakdown.md` for each zone's visual design.

