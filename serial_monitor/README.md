# Serial Monitor with Build Trigger

A Python GUI application that automatically listens to the serial port with the ability to trigger PlatformIO builds, disconnect, upload, and reconnect seamlessly.

## Features

- **Auto Serial Monitoring**: Continuously monitors serial output with timestamps
- **Auto Port Detection**: Automatically detects ESP32/ESP8266 devices
- **Build Trigger**: One-click build and upload with automatic reconnection
- **Real-time Build Output**: Shows PlatformIO build progress as it happens
- **Smart Reconnection**: Handles device resets and reconnections gracefully
- **PlatformIO Integration**: Uses PlatformIO CLI for building and uploading

## Quick Start

1. **Setup** (run once):
   ```bash
   cd serial_monitor
   python setup.py
   ```

2. **Run the application**:
   - **Windows**: Double-click `run_gui.bat`
   - **Linux/Mac**: Run `./run_gui.sh`
   - **Direct**: `python gui_monitor.py`

## Usage

1. **Connect**:
   - Select your serial port from the dropdown (ESP devices are prioritized)
   - Choose baud rate (default: 115200)
   - Click "Connect"

2. **Monitor**:
   - Serial output appears in real-time with timestamps
   - Use "Clear Log" to clear the output window

3. **Build & Upload**:
   - Click "Build & Upload" button
   - Automatically:
     - Clears the log
     - Disconnects serial
     - Runs `platformio run --target upload` with real-time output
     - Waits for device reset
     - Reconnects serial

## Requirements

- Python 3.6+
- PlatformIO CLI (must be in PATH)
- pyserial library (installed automatically by setup.py)

## Installation

### Method 1: Automatic Setup (Recommended)
```bash
cd serial_monitor
python setup.py
```

### Method 2: Manual Setup
```bash
cd serial_monitor
pip install -r requirements.txt
```

## How It Works

1. **Serial Monitoring**: Uses pyserial to continuously read from the serial port
2. **Port Detection**: Scans for available ports and prioritizes ESP32/ESP8266 devices
3. **Build Process**: 
   - Closes serial connection
   - Executes `platformio run --target upload`
   - Waits for device reset (3 seconds)
   - Reopens serial connection
4. **Thread Safety**: GUI version uses threading and queues for safe updates

## Troubleshooting

### PlatformIO Not Found
- Ensure PlatformIO is installed: `pip install platformio`
- Make sure it's in your PATH: `pio --version`

### Serial Port Issues
- Check device is connected and drivers are installed
- Try different USB cables/ports
- Ensure no other applications are using the serial port

### Permission Issues (Linux/Mac)
- Add user to dialout group: `sudo usermod -a -G dialout $USER`
- Log out and back in for changes to take effect

### Build Failures
- Ensure you're in a valid PlatformIO project directory
- Check `platformio.ini` configuration
- Verify target board is correctly specified

## File Structure

```
serial_monitor/
├── gui_monitor.py       # Main GUI application
├── setup.py            # Setup script
├── requirements.txt    # Python dependencies
├── README.md          # This file
├── run_gui.bat        # Windows launcher
└── run_gui.sh         # Unix launcher
```

## Advanced Usage

### Custom Build Commands
Modify the `subprocess.Popen` call in the GUI script to use custom build commands:

```python
# Example: Build specific environment
process = subprocess.Popen(
    ["platformio", "run", "-e", "esp32dev", "--target", "upload"],
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
    text=True,
    bufsize=1,
    universal_newlines=True
)
```

### Multiple Environments
For projects with multiple environments, you can modify the build command or create separate scripts for each environment.

### Custom Serial Settings
The GUI supports different baud rates and can be easily modified for other serial settings like parity, stop bits, etc.

## Contributing

Feel free to submit issues and enhancement requests!

## License

This project is provided as-is for development convenience.