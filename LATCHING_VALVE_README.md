# Latching Valve Control Mode

This document describes the implementation of latching valve control mode (Mode 6) for the WiFi device system.

## Overview

Latching valves are electrically controlled valves that change state (open/close) when a voltage pulse is applied. Unlike standard valves that require continuous power to maintain position, latching valves mechanically "latch" in position and only need a brief electrical pulse to change state.

## Hardware Requirements

- H-bridge driver circuit connected to:
  - **AUX_PIN (GPIO 5)**: Controls one side of H-bridge
  - **SENSE_POWER_PIN (GPIO 14)**: Controls other side of H-bridge
- Latching valve connected to H-bridge output
- Current limiting resistors (recommended)

## Pin Configuration

When device is set to Mode 6 (Latching Valve):

### Opening Valve (Positive Pulse)
- AUX_PIN = HIGH
- SENSE_POWER_PIN = LOW
- Duration: 100ms
- Return to neutral: AUX_PIN = LOW

### Closing Valve (Negative Pulse)
- AUX_PIN = LOW  
- SENSE_POWER_PIN = HIGH
- Duration: 100ms
- Return to neutral: SENSE_POWER_PIN = LOW

## Safety Features

- Mode validation: Valve functions only work when device is in Mode 6
- Pulse duration limited to 100ms to prevent overheating
- Pins never set HIGH simultaneously to prevent H-bridge short circuit
- Automatic return to neutral state after pulse

## API Commands

### Firmware Endpoints
- `POST /output-on` → Opens valve (when in Mode 6)
- `POST /output-off` → Closes valve (when in Mode 6)

### Server Commands
- `valve-open` → Maps to `/output-on` endpoint
- `valve-close` → Maps to `/output-off` endpoint

## Web Interface

### Device Configuration
- Mode 6: "Latching Valve" option available in device setup

### Device Control Page
When device is in Mode 6, shows valve-specific controls:
- **Open Valve** button (with water drop icon)
- **Close Valve** button (with block icon)
- **Schedule Open/Close** options

### Direct Device Interface
The device's built-in control page automatically detects Mode 6 and shows:
- "Open Valve" and "Close Valve" buttons instead of standard On/Off controls

## Implementation Files Modified

### Firmware (ESP8266)
- `src/DeviceManager.h` - Added Mode 6 constant and valve control methods
- `src/DeviceManager.cpp` - Implemented H-bridge control logic
- `src/WebServerManager.cpp` - Route handling for valve commands
- `html/configure.html` - Added Mode 6 option
- `html/control.html` - Valve-specific UI controls

### Server (Deno/TypeScript)
- `server/src/types/device.ts` - Added "Latching Valve" to device modes
- `server/src/types/command.ts` - Added valve-open/valve-close commands
- `server/src/managers/DeviceManager.ts` - Extended command support
- `server/src/managers/CommandQueue.ts` - Command routing for valve operations
- `server/views/pages/device.eta` - Conditional UI for valve controls
- `server/static/js/app.js` - User-friendly valve command notifications

## Usage Example

1. Configure device with Mode 6 (Latching Valve)
2. Connect H-bridge circuit to AUX_PIN and SENSE_POWER_PIN
3. Connect latching valve to H-bridge output
4. Use web interface or API to send valve-open/valve-close commands
5. Device will pulse appropriate pins to actuate valve

## Troubleshooting

- **Valve not responding**: Check H-bridge connections and power supply
- **Valve operates in wrong direction**: Swap H-bridge output connections
- **Commands not working**: Verify device is in Mode 6
- **Overheating**: Check current limiting resistors and pulse duration

## Technical Notes

- Pulse duration is fixed at 100ms (adjustable in firmware if needed)
- State tracking assumes valve reaches commanded position
- No position feedback - commands are open-loop
- Compatible with existing command queue and scheduling system