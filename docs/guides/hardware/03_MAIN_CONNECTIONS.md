---
title: Main Connections
parent: Hardware Guides
nav_order: 4
---

# Main Connections

This guide covers the main power and signal connections between components in the Chelonian Access system.

## Required Components

- Mini360 Buck Converter
- Jumper wires
- Common ground wire
- Wire strippers
- Heat shrink tubing
- Multimeter

## Complete Wiring Overview

![Complete Wiring](../assets/img/assembly/wiring completed.jpeg)
*Complete wiring assembly showing all connections*

## Wire Color Scheme

### Power Distribution

- **Red** - 12V power input
- **Red** - 5V power distribution
- **Black** - Ground connections
- **Black** - Secondary ground connections

### Control Signals

- **Brown** - SPI MISO (PN532)
- **Orange** - SPI MOSI (PN532)
- **Green** - SPI SCK (PN532)
- **Yellow** - SPI SS/CS (PN532)

### Relay Control

- **Brown** - Relay 1 control
- **Orange** - Relay 2 control
- **Yellow** - Relay 3 control
- **Green** - Relay 4 control

## Connection Steps

### 1. Power Distribution Setup

1. **Buck Converter Connections:**

```txt
Mini360 Buck Converter
┌─────────────────┐
│  IN+ ────────── │──── 12V Source (+)
│  IN- ────────── │──── Ground Bus
│  OUT+ ───────── │──── 5V Distribution
│  OUT- ───────── │──── Ground Bus
└─────────────────┘
```

### Best Practices

- Keep power traces short
- Use proper wire gauge
- Add strain relief

### Testing

1. Check voltage at all points
2. Verify ground connections

### Troubleshooting

- Verify all ground connections
- Check for voltage drops
- Look for loose connections

[Return to Assembly Guide](ASSEMBLY_GUIDE.md) - Return to the Guide