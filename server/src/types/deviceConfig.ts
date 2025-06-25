export interface WiFiNetwork {
  ssid: string;
  encryption: string;
  rssi: number;
}

export interface DeviceConfiguration {
  ssid: string;
  password?: string; // Optional for security - not returned in responses
  alias: string;
  server: string;
  mode: number;
}

export interface DeviceConfigResponse {
  networks: WiFiNetwork[];
  storedSsid: string;
  alias: string;
  server: string;
  mode: number;
}

export interface ConfigureDeviceRequest {
  ssid: string;
  password: string;
  alias: string;
  server: string;
  mode: string; // Comes as string from form data
}

export interface ConfigurationComparison {
  ssid: boolean;
  alias: boolean;
  server: boolean;
  mode: boolean;
  hasChanges: boolean;
}

export const DeviceMode = {
  SERVO: 0,
  INPUT_SWITCH: 1,
  THERMOMETER: 2,
  SOIL_SENSOR: 3,
  RELAY: 4,
  RGB_LED: 5,
  LATCHING_VALVE: 6
} as const;

export type DeviceModeType = typeof DeviceMode[keyof typeof DeviceMode];

export function isValidDeviceMode(mode: number): mode is DeviceModeType {
  return Object.values(DeviceMode).includes(mode as DeviceModeType);
}

export function compareConfigurations(
  current: DeviceConfiguration, 
  incoming: DeviceConfiguration
): ConfigurationComparison {
  const comparison: ConfigurationComparison = {
    ssid: current.ssid === incoming.ssid,
    alias: current.alias === incoming.alias,
    server: current.server === incoming.server,
    mode: current.mode === incoming.mode,
    hasChanges: false
  };
  
  comparison.hasChanges = !comparison.ssid || !comparison.alias || 
                         !comparison.server || !comparison.mode;
  
  return comparison;
}