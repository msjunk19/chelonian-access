# JQ6500 Audio File Installation Guide

This guide explains how to install the required audio files onto your JQ6500 audio module for use with the Chelonian Access system.

## Required Files

The system uses 6 distinct audio files for different states:
- Access Granted
- Access Denied
- Error
- Programming Mode
- Programming Success
- Programming Failed

## Prerequisites

- [JQ6500 Mini USB Download Tool](https://github.com/NikolaiRadke/JQ6500-rescue-tool) (for Linux/Windows)
- USB-TTL adapter (3.3V logic level)
- Audio files (available in the `docs/assets/audio` directory)

## Installation Methods

### Method 1: Linux - USB Download Tool (Recommended)

1. **Connect the JQ6500**
   - Remove the JQ6500 module from your Chelonian Access board
   - Connect it to your computer via a USB-TTL adapter:
     - TX → RX
     - RX → TX
     - GND → GND
     - VCC → 3.3V

2.  **Install the Download Tool**
   ```bash
   # Linux
   git clone https://github.com/NikolaiRadke/JQ6500-rescue-tool.git
   cd JQ6500-rescue-tool
   make
   sudo make install
   ```

3. **Prepare Audio Files**
   - Copy the audio files from `docs/assets/audio` to a working directory
   - Ensure files are in MP3 format and properly named:
     - `001.mp3` - Startup Sound/Chime
     - `002.mp3` - Waiting for Input
     - `003.mp3` - Access Granted
     - `004.mp3` - Acccess Denied 1
     - `005.mp3` - Acccess Denied 2
     - `006.mp3` - Acccess Denied 3
   - You must add ALL of the files, at once, in numerical order (sorting)

4. **Upload Files**
   ```bash
   # Linux
   jq6500-tool -d /dev/ttyUSB0 -f /path/to/audio/files/*.mp3
   ```

### Method 2: Windows - JQ6500s Tool

For Windows users, an alternative GUI tool is available:

The device should show up on your PC as a cd drive, on it will be MusicDownloader.exe, use the directions below to use it. Alternatively, if it is not installed, you can download it as referenced.

1. Download the JQ6500s tool from the manufacturer's website
2. Connect the module as described above
3. Open the JQ6500s tool
4. Click "Connect Device"
5. Add your audio files in the correct order
6. Click "Download to Device"

## Verification

1. After installation, reconnect the JQ6500 to your Chelonian Access board
2. Power up the system
3. Test each sound:
   - Present a valid card (Access Granted)
   - Present an invalid card (Access Denied)
   - Enter programming mode (Programming Mode sound)
   - etc.

## Troubleshooting

### Common Issues

1. **Module Not Detected**
   - Check USB-TTL connections
   - Verify adapter is set to 3.3V
   - Try a different USB port

2. **Upload Fails**
   - Ensure files are proper MP3 format
   - Check file naming (must be 001.mp3, 002.mp3, etc.)
   - Verify USB-TTL adapter is working correctly

3. **No Sound After Installation**
   - Check volume setting on module
   - Verify speaker connections
   - Test with a known working audio file

## Additional Resources

- [JQ6500 Datasheet](link-to-datasheet)
- [Original Manufacturer Tools](link-to-tools)
- [Alternative Programming Methods](link-to-alternatives)
