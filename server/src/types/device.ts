export interface ContactRecord {
  timestamp: Date;
  ipAddress: string;
  action: string;
}

export interface SensorReading {
  timestamp: Date;
  type: 'temperature' | 'soil_moisture' | 'analog';
  value: number;
  unit?: string;
}

export interface DeviceState {
  id: string;                    // Serial number from device
  alias: string;                 // User-friendly name
  ipAddress: string;
  macAddress: string;
  firmwareVersion?: string;
  mode: number;                  // Operating mode (0-5)
  isOnline: boolean;
  lastSeen: Date;
  contactHistory: ContactRecord[];
  currentOutput: boolean;        // Current on/off state
  sensorData: SensorReading[];
  pendingCommands: string[];     // Command IDs
  sleepStatus: 'awake' | 'asleep' | 'unknown';  // Current sleep state
  forceAwake: boolean;           // Manual stay-awake override
  lastAwakeCheck: Date;          // Last time device checked if it should stay awake
}

export interface SystemStats {
  totalDevices: number;
  onlineDevices: number;
  pendingCommands: number;
  uptime: number;
  lastUpdate: Date;
}

export interface SystemState {
  devices: Map<string, DeviceState>;
  systemStats: SystemStats;
}

export interface SerializableSystemState {
  devices: Record<string, DeviceState>;
  systemStats: SystemStats;
}

export const DEVICE_MODES = {
  0: 'Servo',
  1: 'Input Switch',
  2: 'Thermometer',
  3: 'Soil Sensor',
  4: 'Relay',
  5: 'RGB LED',
  6: 'Latching Valve'
} as const;

export type DeviceMode = keyof typeof DEVICE_MODES;

export interface WiFiFailure {
  timestamp: Date;
  ssid: string;
  reason: string;
}

export interface DeviceRegistration {
  id: string;
  alias: string;
  ipAddress: string;
  macAddress: string;
  mode: number;
}

export interface WiFiFailureReport {
  id: string;
  alias: string;
  failures: string; // JSON string from device
}