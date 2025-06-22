import { DeviceState, SystemState, SystemStats, ContactRecord } from "../types/device.ts";
import { Command } from "../types/command.ts";

export class StateManager {
  private state: SystemState;
  private listeners: Set<(state: SystemState) => void> = new Set();
  private commands: Map<string, Command> = new Map();
  private startTime: Date = new Date();

  constructor() {
    this.state = {
      devices: new Map(),
      systemStats: {
        totalDevices: 0,
        onlineDevices: 0,
        pendingCommands: 0,
        uptime: 0,
        lastUpdate: new Date()
      }
    };
    // Now that state is initialized, calculate the actual stats
    this.updateStats();
  }

  // Device Management
  registerDevice(deviceData: {
    id: string;
    alias: string;
    ipAddress: string;
    macAddress: string;
    mode: number;
  }): void {
    const now = new Date();
    const existingDevice = this.state.devices.get(deviceData.id);
    
    const contactRecord: ContactRecord = {
      timestamp: now,
      ipAddress: deviceData.ipAddress,
      action: 'register'
    };

    const device: DeviceState = {
      id: deviceData.id,
      alias: deviceData.alias,
      ipAddress: deviceData.ipAddress,
      macAddress: deviceData.macAddress,
      mode: deviceData.mode,
      isOnline: true,
      lastSeen: now,
      contactHistory: existingDevice ? 
        [...existingDevice.contactHistory.slice(-49), contactRecord] : 
        [contactRecord],
      currentOutput: existingDevice?.currentOutput ?? false,
      sensorData: existingDevice?.sensorData ?? [],
      pendingCommands: existingDevice?.pendingCommands ?? []
    };

    this.state.devices.set(deviceData.id, device);
    this.updateStats();
    this.notifyListeners();
  }

  updateDeviceContact(deviceId: string, action: string): void {
    const device = this.state.devices.get(deviceId);
    if (!device) return;

    const contactRecord: ContactRecord = {
      timestamp: new Date(),
      ipAddress: device.ipAddress,
      action
    };

    device.lastSeen = new Date();
    device.isOnline = true;
    device.contactHistory = [...device.contactHistory.slice(-49), contactRecord];
    
    this.updateStats();
    this.notifyListeners();
  }

  updateDeviceAlias(deviceId: string, alias: string): boolean {
    const device = this.state.devices.get(deviceId);
    if (!device) return false;

    device.alias = alias;
    this.notifyListeners();
    return true;
  }

  updateDeviceOutput(deviceId: string, outputState: boolean): void {
    const device = this.state.devices.get(deviceId);
    if (!device) return;

    device.currentOutput = outputState;
    this.notifyListeners();
  }

  // Command Management
  addCommand(command: Command): void {
    this.commands.set(command.id, command);
    
    const device = this.state.devices.get(command.deviceId);
    if (device) {
      device.pendingCommands.push(command.id);
    }
    
    this.updateStats();
    this.notifyListeners();
  }

  updateCommand(commandId: string, updates: Partial<Command>): void {
    const command = this.commands.get(commandId);
    if (!command) return;

    Object.assign(command, updates);
    
    // If command is completed or failed, remove from device pending list
    if (updates.status === 'completed' || updates.status === 'failed') {
      const device = this.state.devices.get(command.deviceId);
      if (device) {
        device.pendingCommands = device.pendingCommands.filter(id => id !== commandId);
      }
    }
    
    this.updateStats();
    this.notifyListeners();
  }

  getCommand(commandId: string): Command | undefined {
    return this.commands.get(commandId);
  }

  getPendingCommandsForDevice(deviceId: string): Command[] {
    const device = this.state.devices.get(deviceId);
    if (!device) return [];

    return device.pendingCommands
      .map(id => this.commands.get(id))
      .filter((cmd): cmd is Command => cmd !== undefined)
      .filter(cmd => cmd.status === 'pending' && new Date() >= cmd.scheduledFor);
  }

  getAllCommands(): Command[] {
    return Array.from(this.commands.values());
  }

  // Device Status Management
  checkDeviceStatus(): void {
    const now = new Date();
    const offlineThreshold = 5 * 60 * 1000; // 5 minutes
    let statusChanged = false;

    for (const device of this.state.devices.values()) {
      const timeSinceLastSeen = now.getTime() - device.lastSeen.getTime();
      const wasOnline = device.isOnline;
      device.isOnline = timeSinceLastSeen < offlineThreshold;
      
      if (wasOnline !== device.isOnline) {
        statusChanged = true;
      }
    }

    if (statusChanged) {
      this.updateStats();
      this.notifyListeners();
    }
  }

  // State Access
  getState(): SystemState {
    return {
      devices: new Map(this.state.devices),
      systemStats: { ...this.state.systemStats }
    };
  }

  getDevice(deviceId: string): DeviceState | undefined {
    const device = this.state.devices.get(deviceId);
    return device ? { ...device } : undefined;
  }

  getAllDevices(): DeviceState[] {
    return Array.from(this.state.devices.values()).map(device => ({ ...device }));
  }

  // Listeners
  addListener(listener: (state: SystemState) => void): void {
    this.listeners.add(listener);
  }

  removeListener(listener: (state: SystemState) => void): void {
    this.listeners.delete(listener);
  }

  private notifyListeners(): void {
    const state = this.getState();
    this.listeners.forEach(listener => {
      try {
        listener(state);
      } catch (error) {
        console.error('Error in state listener:', error);
      }
    });
  }

  private updateStats(): void {
    this.state.systemStats = this.calculateStats();
  }

  private calculateStats(): SystemStats {
    const devices = Array.from(this.state.devices.values());
    const commands = Array.from(this.commands.values());
    
    return {
      totalDevices: devices.length,
      onlineDevices: devices.filter(d => d.isOnline).length,
      pendingCommands: commands.filter(c => c.status === 'pending').length,
      uptime: Date.now() - this.startTime.getTime(),
      lastUpdate: new Date()
    };
  }

  // Cleanup
  cleanupOldCommands(maxAge: number = 24 * 60 * 60 * 1000): void {
    const cutoff = new Date(Date.now() - maxAge);
    const commandsToDelete: string[] = [];

    for (const [id, command] of this.commands) {
      if (command.createdAt < cutoff && 
          (command.status === 'completed' || command.status === 'failed')) {
        commandsToDelete.push(id);
      }
    }

    commandsToDelete.forEach(id => this.commands.delete(id));
    
    if (commandsToDelete.length > 0) {
      this.updateStats();
      this.notifyListeners();
    }
  }
}