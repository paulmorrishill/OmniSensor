# WiFi Device Management Server

A comprehensive Deno TypeScript server for managing ESP8266 WiFi sensor devices with real-time web interface, command queuing, and device lifecycle management.

## Features

- **Device Management**: Automatic device discovery and registration
- **Real-time Updates**: WebSocket-powered live dashboard
- **Command Queuing**: Smart command execution with scheduling
- **Material Design**: Clean, responsive web interface
- **Single Source of Truth**: Centralized state management
- **Device Control**: On/off control with scheduling capabilities
- **Device Renaming**: Easy device alias management
- **Offline Support**: Commands queued when devices are sleeping

## Architecture

### Core Components

- **StateManager**: Single source of truth for all device and system state
- **CommandQueue**: Handles command scheduling and execution with retry logic
- **DeviceManager**: Manages device lifecycle and communication
- **WebSocketHandler**: Real-time updates to connected clients

### API Endpoints

#### Device Communication (for ESP8266 devices)
- `POST /register` - Device registration
- `GET /should-remain-awake?id={deviceId}` - Sleep/wake control
- `POST /wifi-failures` - WiFi failure reporting

#### Web API (for frontend)
- `GET /api/devices` - Get all devices
- `GET /api/devices/:id` - Get specific device
- `POST /api/devices/:id/control` - Control device output
- `POST /api/devices/:id/rename` - Rename device
- `POST /api/devices/wake-all` - Wake all devices
- `POST /api/devices/sleep-all` - Sleep all devices

#### Web Interface
- `GET /` - Dashboard
- `GET /device/:id` - Device details
- `GET /settings` - System settings

## Installation

### Prerequisites

- [Deno](https://deno.land/) 1.37+ installed
- ESP8266 devices with compatible firmware

### Setup

1. Clone or create the server directory:
```bash
mkdir wifi-device-server
cd wifi-device-server
```

2. Copy all server files to the directory

3. Start the server:
```bash
deno task start
```

Or for development with auto-reload:
```bash
deno task dev
```

## Configuration

### Environment Variables

- `PORT` - Server port (default: 8000)

### Device Firmware Compatibility

The server is compatible with ESP8266 devices that send:

**Registration Request** (`POST /register`):
```json
{
  "id": "LT1AABBCCDDEEFF12345",
  "alias": "Device Name",
  "ipAddress": "192.168.1.100",
  "macAddress": "AA:BB:CC:DD:EE:FF",
  "mode": 2
}
```

**Should Remain Awake** (`GET /should-remain-awake?id=deviceId`):
- Returns "1" to stay awake, "0" to sleep

**WiFi Failures** (`POST /wifi-failures`):
```json
{
  "id": "LT1AABBCCDDEEFF12345",
  "alias": "Device Name",
  "failures": "[{\"timestamp\":\"...\",\"ssid\":\"...\",\"reason\":\"...\"}]"
}
```

## Device Control

### Command Types

- `output-on` - Turn device output on
- `output-off` - Turn device output off  
- `one-sec-on` - Turn on for 1 second then off

### Command Scheduling

Commands can be scheduled for future execution:

```javascript
// Schedule command for 5 minutes from now
fetch('/api/devices/device123/control', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({
    action: 'output-off',
    scheduleDelay: 5 * 60 * 1000 // 5 minutes in milliseconds
  })
});
```

## WebSocket API

Connect to `ws://localhost:8000/ws` for real-time updates.

### Message Types

**State Updates** (server to client):
```json
{
  "type": "full-state",
  "timestamp": "2023-12-07T10:30:00.000Z",
  "data": {
    "devices": {...},
    "systemStats": {...}
  }
}
```

**Ping/Pong** (bidirectional):
```json
{
  "type": "ping",
  "timestamp": "2023-12-07T10:30:00.000Z"
}
```

## Device Modes

The system supports various device operating modes:

- **0**: Servo control
- **1**: Input switch
- **2**: Thermometer
- **3**: Soil sensor
- **4**: Relay control
- **5**: RGB LED control

## Development

### Project Structure

```
server/
├── deno.json              # Deno configuration
├── main.ts                # Application entry point
├── src/
│   ├── types/             # TypeScript type definitions
│   ├── managers/          # Core business logic
│   ├── api/               # HTTP route handlers
│   ├── websocket/         # WebSocket handling
│   ├── middleware/        # HTTP middleware
│   └── utils/             # Utility functions
├── views/                 # ETA templates
├── static/                # Static assets (CSS, JS)
└── README.md
```

### Adding New Features

1. **New Device Commands**: Add to `CommandType` in `types/command.ts` and implement in `CommandQueue.ts`
2. **New API Endpoints**: Add routes in `api/apiRoutes.ts`
3. **UI Components**: Create ETA templates in `views/`
4. **WebSocket Messages**: Extend message handling in `websocket/wsHandler.ts`

### Testing

```bash
# Run tests (when available)
deno task test

# Type checking
deno check main.ts

# Linting
deno lint

# Formatting
deno fmt
```

## Deployment

### Production Deployment

1. Set environment variables:
```bash
export PORT=8000
```

2. Run with production settings:
```bash
deno run --allow-net --allow-read --allow-write main.ts
```

### Docker Deployment

Create a `Dockerfile`:
```dockerfile
FROM denoland/deno:1.37.0

WORKDIR /app
COPY . .

EXPOSE 8000

CMD ["run", "--allow-net", "--allow-read", "--allow-write", "main.ts"]
```

### Systemd Service

Create `/etc/systemd/system/wifi-device-server.service`:
```ini
[Unit]
Description=WiFi Device Management Server
After=network.target

[Service]
Type=simple
User=wifi-server
WorkingDirectory=/opt/wifi-device-server
ExecStart=/usr/local/bin/deno run --allow-net --allow-read --allow-write main.ts
Restart=always
Environment=PORT=8000

[Install]
WantedBy=multi-user.target
```

## Troubleshooting

### Common Issues

1. **WebSocket Connection Failed**
   - Check firewall settings
   - Verify port is not in use
   - Check browser console for errors

2. **Devices Not Registering**
   - Verify device firmware is sending correct JSON format
   - Check network connectivity
   - Review server logs for registration errors

3. **Commands Not Executing**
   - Ensure devices are online
   - Check command queue status in dashboard
   - Verify device IP addresses are reachable

### Logging

The server logs important events to console. For production, consider redirecting to files:

```bash
deno run --allow-net --allow-read --allow-write main.ts > server.log 2>&1
```

## License

This project is part of the WiFi sensor ecosystem. See main project for license details.