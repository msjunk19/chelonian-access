---
title: Power Supply Setup
parent: Hardware Guides
nav_order: 2
---

# Power Supply Setup

This guide covers the setup and configuration of the power supply components for the Chelonian Access system.

![Mini360 Buck Converter](../assets/img/assembly/mini360 buck converter.jpg)
*Mini360 Buck Converter Module*

## Required Components

- Mini360 Buck Converter
- 12V power source
- Multimeter
- Wire strippers
- Heat shrink tubing

## Configuration Steps

### 1. Mini360 Buck Converter Setup

1. **Initial Inspection:**
   - Inspect the Mini360 module for any damage
   - Identify IN+, IN-, OUT+, and OUT- terminals
   - Locate the voltage adjustment trim pot

2. **Voltage Configuration:**
   - Connect a multimeter to OUT+ and OUT-
   - Apply 12V to IN+ and IN- (from vehicle or power supply)
   - Adjust the trim pot until output reads exactly 5.0V
   - Double-check voltage stability under load

### Best Practices

- Add heat shrink tubing to exposed connections
- Consider adding inline fuse protection

### Testing

1. Check output voltage with no load
2. Verify voltage remains stable under load
3. Test for any voltage ripple if possible
4. Ensure converter doesn't get too hot

### Troubleshooting

- If output voltage is unstable, check input voltage stability
- If converter gets hot, ensure proper ventilation
- If output is 0V, check input connections and polarity

[Return to Assembly Guide](ASSEMBLY_GUIDE.md) - Return to the Guide