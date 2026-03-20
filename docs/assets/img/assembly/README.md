# Pre-Assembly Component Photography Guidelines

This directory contains guidelines for photographing the unassembled components of the Chelonian Access system. These photos should show the components and their connections before any soldering or permanent assembly.

## Required Component Photos

1. `01_components.jpg` - Components Layout
   - All components laid out neatly
   - Labels or arrows indicating each component
   - Include all required tools
   - Show wire colors for reference

2. `02_power_setup.jpg` - Power Supply Setup
   - Mini360 buck converter connections
   - Multimeter showing 5V output
   - Input/output wire colors (Red/Black standard)
   - Clear view of trim pot adjustment

3. `03_core_connections.jpg` - ESP32 Core Setup
   - ESP32-C3 SuperMini with initial connections
   - Power connections (Red: 5V, Black: GND)
   - Clear view of pin numbers
   - Initial SPI connections for PN532

4. `04_rfid_setup.jpg` - RFID Module
   - PN532 module connections
   - Jumper settings for SPI mode
   - Wire colors following standard:
     * Red: VCC (3.3V)
     * Black: GND
     * Brown: MISO
     * Orange: MOSI
     * Yellow: SCK
     * Green: SS

5. `05_relay_setup.jpg` - Relay Module
   - Complete relay module connections
   - Wire colors following standard:
     * Red: VCC (5V)
     * Black: GND
     * Brown: IN1 (Door)
     * Orange: IN2
     * Yellow: IN3
     * Green: IN4

6. `06_audio_setup.jpg` - Audio Setup
   - JQ6500 module connections
   - Speaker mounting
   - Wire colors following standard:
     * Red: VCC (5V)
     * Black: GND
     * Brown: TX
     * Orange: RX

7. `07_final_assembly.jpg` - Complete Assembly
   - Final enclosure mounting
   - Cable management
   - Component spacing
   - Overview of complete system

## Photo Requirements

### Technical Requirements
- Resolution: Minimum 2048x1536 pixels
- Format: JPG
- File size: < 500KB per image
- Focus: Sharp, clear focus on connections
- Lighting: Even, bright lighting without glare

### Composition Guidelines
1. Include a ruler or scale reference
2. Use neutral background
3. Position components at 45° angle for best visibility
4. Include arrow overlays for small details
5. Show before/after for complex connections

### Wire Color Standard
Always follow the standard wire colors:
- Power (VCC): Red
- Ground (GND): Black
- Signal wires (in position order):
  1. Brown (Position 0)
  2. Orange (Position 1)
  3. Yellow (Position 2)
  4. Green (Position 3)
  5. Blue (Position 4)
  6. Purple (Position 5)

### Safety Elements to Show
- Heat shrink on connections
- Proper strain relief
- Fuse placement
- Grounding points
- Spacing between components

## Image Processing
1. Crop to 4:3 aspect ratio
2. Optimize for web viewing
3. Add labels or arrows in post-processing
4. Include scale bar where relevant
5. Compress to reduce file size

## Directory Structure
```
assembly/
├── 01_components.jpg
├── 02_power_setup.jpg
├── 03_core_connections.jpg
├── 04_rfid_setup.jpg
├── 05_relay_setup.jpg
├── 06_audio_setup.jpg
└── 07_final_assembly.jpg
```

## Updating Photos
When updating photos:
1. Maintain consistent naming convention
2. Archive old versions if needed
3. Update ASSEMBLY_GUIDE.md if perspective changes
4. Ensure all wire colors match current standard
5. Verify all safety elements are visible
