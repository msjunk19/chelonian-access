# ESP32-C3 Firmware Installation Guide

This guide will walk you through the process of installing the Chelonian Access firmware onto your ESP32-C3 SuperMini board using PlatformIO.

## Prerequisites

- [Visual Studio Code](https://code.visualstudio.com/) installed
- [PlatformIO IDE Extension](https://platformio.org/install/ide?install=vscode) installed
- USB-C cable for connecting to the ESP32-C3 SuperMini

## Installation Steps

1. **Clone the Repository**
   ```bash
   git clone https://github.com/msjunk19/chelonian-access.git
   cd chelonian-access
   ```

2. **Open in VS Code with PlatformIO**
   - Open VS Code
   - Click "Open Folder" and select the cloned `chelonian-access` directory
   - PlatformIO will automatically detect the project configuration

3. **Connect the ESP32-C3**
   - Connect your ESP32-C3 SuperMini to your computer via USB-C
   - The board should appear as a serial port device
   - Note: On Linux, you may need to add your user to the `dialout` group:
     ```bash
     sudo usermod -a -G dialout $USER
     ```
     (Log out and back in for changes to take effect)

4. **Build and Upload**
   - Click the PlatformIO icon in the VS Code sidebar
   - Under "Project Tasks" > "General", click "Build" to compile
   - Once successful, click "Upload" to flash the firmware
   - The first upload may take longer as PlatformIO downloads required dependencies

## Troubleshooting

### Common Issues

1. **Board Not Detected**
   - Check USB cable connection
   - Verify the board appears in `ls /dev/ttyUSB*` or `ls /dev/ttyACM*`
   - Try a different USB port or cable

2. **Upload Failed**
   - Press and hold the BOOT button while starting the upload
   - Release after upload begins
   - If issues persist, try manually entering download mode:
     1. Press and hold BOOT
     2. Press RST briefly
     3. Release BOOT
     4. Start upload again

3. **Build Errors**
   - Run `pio run -t clean` to clean the build
   - Check that all dependencies are properly installed
   - Verify your PlatformIO is up to date

## Verification

After successful upload:
1. The board's LED should blink according to the programmed pattern
2. You can monitor the serial output in PlatformIO's Serial Monitor
3. Test RFID functionality with a compatible card

## Next Steps

- Proceed to [JQ6500 Audio File Installation](02_JQ6500_AUDIO_INSTALLATION.html) to set up sound effects
- Configure your access cards using the instructions in the [Programming Guide](../programming/CARD_PROGRAMMING.html)
- Review the [Hardware Setup Guide](../hardware/README.html) if you haven't completed the physical assembly
