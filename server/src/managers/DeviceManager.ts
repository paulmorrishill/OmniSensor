import { StateManager } from "./StateManager.ts";
import { CommandQueue } from "./CommandQueue.ts";
import { DeviceRegistration, WiFiFailureReport } from "../types/device.ts";

export class DeviceManager {
  private stateManager: StateManager;
  private commandQueue: CommandQueue;
  private statusCheckInterval: number;
  private intervalId?: number;

  constructor(stateManager: StateManager, commandQueue: CommandQueue) {
    this.stateManager = stateManager;
    this.commandQueue = commandQueue;
    this.statusCheckInterval = 60000; // Check every minute
  }

  start(): void {
    if (this.intervalId) return;
    
    this.intervalId = setInterval(() => {
      this.stateManager.checkDeviceStatus();
      this.stateManager.cleanupOldCommands();
    }, this.statusCheckInterval);
    
    console.log('Device manager started');
  }

  stop(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = undefined;
      console.log('Device manager stopped');
    }
  }

  handleDeviceRegistration(registration: DeviceRegistration): void {
    console.log(`Device registration: ${registration.id} (${registration.alias})`);
    
    this.stateManager.registerDevice({
      id: registration.id,
      alias: registration.alias,
      ipAddress: registration.ipAddress,
      macAddress: registration.macAddress,
      mode: registration.mode
    });
  }

  handleShouldRemainAwake(deviceId: string): boolean {
    this.stateManager.updateDeviceContact(deviceId, 'should-remain-awake');
    
    const device = this.stateManager.getDevice(deviceId);
    if (!device) {
      console.log(`Device ${deviceId} not found`);
      return false;
    }
    
    // Check if device has pending commands or is forced to stay awake
    const pendingCommands = this.stateManager.getPendingCommandsForDevice(deviceId);
    const hasPendingCommands = pendingCommands.length > 0;
    const shouldStayAwake = hasPendingCommands || device.forceAwake;
    
    // Update the device's sleep status based on our decision
    const sleepStatus = shouldStayAwake ? 'awake' : 'asleep';
    this.stateManager.updateDeviceSleepStatus(deviceId, sleepStatus);
    
    console.log(`Device ${deviceId} should remain awake: ${shouldStayAwake} (${pendingCommands.length} pending commands, forceAwake: ${device.forceAwake})`);
    
    return shouldStayAwake;
  }

  handleWiFiFailures(report: WiFiFailureReport): void {
    console.log(`WiFi failure report from ${report.id}: ${report.failures}`);
    
    this.stateManager.updateDeviceContact(report.id, 'wifi-failure-report');
    
    // TODO: Parse and store WiFi failure data if needed
    // For now, we just log the contact
  }

  renameDevice(deviceId: string, newAlias: string): boolean {
    const success = this.stateManager.updateDeviceAlias(deviceId, newAlias);
    
    if (success) {
      console.log(`Device ${deviceId} renamed to: ${newAlias}`);
    } else {
      console.error(`Failed to rename device ${deviceId}: device not found`);
    }
    
    return success;
  }

  controlDevice(deviceId: string, action: 'output-on' | 'output-off' | 'one-sec-on' | 'valve-open' | 'valve-close', scheduleDelay?: number): string | null {
    const device = this.stateManager.getDevice(deviceId);
    if (!device) {
      console.error(`Cannot control device ${deviceId}: device not found`);
      return null;
    }

    const commandId = this.commandQueue.queueCommand(deviceId, {
      type: action,
      scheduleDelay
    });

    console.log(`Queued ${action} command for device ${deviceId} (${device.alias})`);
    return commandId;
  }

  scheduleDeviceAction(deviceId: string, action: 'output-on' | 'output-off' | 'valve-open' | 'valve-close', scheduleFor: Date): string | null {
    const device = this.stateManager.getDevice(deviceId);
    if (!device) {
      console.error(`Cannot schedule action for device ${deviceId}: device not found`);
      return null;
    }

    const commandId = this.commandQueue.queueScheduledCommand(deviceId, action, scheduleFor);
    
    console.log(`Scheduled ${action} for device ${deviceId} (${device.alias}) at ${scheduleFor.toISOString()}`);
    return commandId;
  }

  getDeviceStatus(deviceId: string) {
    const device = this.stateManager.getDevice(deviceId);
    if (!device) return null;

    const pendingCommands = this.stateManager.getPendingCommandsForDevice(deviceId);
    
    return {
      ...device,
      pendingCommandCount: pendingCommands.length,
      lastSeenAgo: Date.now() - device.lastSeen.getTime()
    };
  }

  getAllDevicesStatus() {
    const devices = this.stateManager.getAllDevices();
    
    return devices.map(device => {
      const pendingCommands = this.stateManager.getPendingCommandsForDevice(device.id);
      
      return {
        ...device,
        pendingCommandCount: pendingCommands.length,
        lastSeenAgo: Date.now() - device.lastSeen.getTime()
      };
    });
  }

  getSystemHealth() {
    const state = this.stateManager.getState();
    const queueStats = this.commandQueue.getQueueStats();
    
    return {
      devices: {
        total: state.systemStats.totalDevices,
        online: state.systemStats.onlineDevices,
        offline: state.systemStats.totalDevices - state.systemStats.onlineDevices
      },
      commands: queueStats,
      uptime: state.systemStats.uptime,
      lastUpdate: state.systemStats.lastUpdate
    };
  }

  // Bulk operations
  wakeAllDevices(): string[] {
    const devices = this.stateManager.getAllDevices();
    const commandIds: string[] = [];
    
    for (const device of devices) {
      if (device.isOnline) {
        const commandId = this.controlDevice(device.id, 'output-on');
        if (commandId) {
          commandIds.push(commandId);
        }
      }
    }
    
    console.log(`Queued wake commands for ${commandIds.length} devices`);
    return commandIds;
  }

  sleepAllDevices(): string[] {
    const devices = this.stateManager.getAllDevices();
    const commandIds: string[] = [];
    
    for (const device of devices) {
      if (device.isOnline && device.currentOutput) {
        const commandId = this.controlDevice(device.id, 'output-off');
        if (commandId) {
          commandIds.push(commandId);
        }
      }
    }
    
    console.log(`Queued sleep commands for ${commandIds.length} devices`);
    return commandIds;
  }

  // Command management
  cancelCommand(commandId: string): boolean {
    return this.commandQueue.cancelCommand(commandId);
  }

  retryCommand(commandId: string): boolean {
    return this.commandQueue.retryCommand(commandId);
  }

  // Force awake management
  setDeviceForceAwake(deviceId: string, forceAwake: boolean): boolean {
    const device = this.stateManager.getDevice(deviceId);
    if (!device) {
      console.error(`Cannot set force awake for device ${deviceId}: device not found`);
      return false;
    }

    const success = this.stateManager.setDeviceForceAwake(deviceId, forceAwake);
    
    if (success) {
      console.log(`Device ${deviceId} (${device.alias}) force awake set to: ${forceAwake}`);
    }
    
    return success;
  }

  toggleDeviceForceAwake(deviceId: string): boolean {
    const device = this.stateManager.getDevice(deviceId);
    if (!device) {
      console.error(`Cannot toggle force awake for device ${deviceId}: device not found`);
      return false;
    }

    const newForceAwake = !device.forceAwake;
    return this.setDeviceForceAwake(deviceId, newForceAwake);
  }
}