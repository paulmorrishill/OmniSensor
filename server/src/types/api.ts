import { DeviceState, SystemState } from "./device.ts";
import { Command } from "./command.ts";

export interface ApiResponse<T = any> {
  success: boolean;
  data?: T;
  error?: string;
  timestamp: Date;
}

export interface StateMessage {
  type: 'full-state';
  timestamp: Date;
  data: SystemState;
}

export interface WebSocketMessage {
  type: string;
  payload: any;
  timestamp: Date;
}

export interface DeviceControlRequest {
  action: 'output-on' | 'output-off' | 'one-sec-on';
  scheduleDelay?: number; // milliseconds
}

export interface DeviceRenameRequest {
  alias: string;
}

export interface DeviceModeRequest {
  mode: number;
}

export interface DeviceForceAwakeRequest {
  forceAwake: boolean;
}

export interface PaginatedResponse<T> {
  items: T[];
  total: number;
  page: number;
  pageSize: number;
  hasNext: boolean;
  hasPrev: boolean;
}

export interface DeviceListResponse extends PaginatedResponse<DeviceState> {}

export interface CommandListResponse extends PaginatedResponse<Command> {}

export interface HealthCheckResponse {
  status: 'healthy' | 'degraded' | 'unhealthy';
  uptime: number;
  version: string;
  timestamp: Date;
  services: {
    webServer: boolean;
    websocket: boolean;
    commandQueue: boolean;
    deviceManager: boolean;
  };
}

export interface ErrorResponse {
  error: string;
  code: string;
  details?: any;
  timestamp: Date;
}