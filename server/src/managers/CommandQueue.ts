import { Command, CommandType, CommandRequest, CommandResult } from "../types/command.ts";
import { StateManager } from "./StateManager.ts";

export class CommandQueue {
  private stateManager: StateManager;
  private executionInterval: number;
  private intervalId?: number;

  constructor(stateManager: StateManager, executionInterval = 5000) {
    this.stateManager = stateManager;
    this.executionInterval = executionInterval;
  }

  start(): void {
    if (this.intervalId) return;
    
    this.intervalId = setInterval(() => {
      this.processCommands();
    }, this.executionInterval);
    
    console.log('Command queue started');
  }

  stop(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = undefined;
      console.log('Command queue stopped');
    }
  }

  queueCommand(deviceId: string, request: CommandRequest): string {
    const commandId = this.generateCommandId();
    const now = new Date();
    const scheduledFor = request.scheduleDelay 
      ? new Date(now.getTime() + request.scheduleDelay)
      : now;

    const command: Command = {
      id: commandId,
      deviceId,
      type: request.type,
      payload: request.payload,
      scheduledFor,
      createdAt: now,
      attempts: 0,
      maxAttempts: 3,
      status: 'pending'
    };

    this.stateManager.addCommand(command);
    console.log(`Queued command ${commandId} for device ${deviceId}: ${request.type}`);
    
    return commandId;
  }

  queueScheduledCommand(deviceId: string, type: CommandType, scheduleFor: Date, payload?: any): string {
    const commandId = this.generateCommandId();
    
    const command: Command = {
      id: commandId,
      deviceId,
      type,
      payload,
      scheduledFor: scheduleFor,
      createdAt: new Date(),
      attempts: 0,
      maxAttempts: 3,
      status: 'pending'
    };

    this.stateManager.addCommand(command);
    console.log(`Scheduled command ${commandId} for device ${deviceId}: ${type} at ${scheduleFor.toISOString()}`);
    
    return commandId;
  }

  private async processCommands(): Promise<void> {
    const devices = this.stateManager.getAllDevices();
    
    for (const device of devices) {
      if (!device.isOnline) continue;
      
      const pendingCommands = this.stateManager.getPendingCommandsForDevice(device.id);
      
      for (const command of pendingCommands) {
        await this.executeCommand(command);
      }
    }
  }

  private async executeCommand(command: Command): Promise<void> {
    const device = this.stateManager.getDevice(command.deviceId);
    if (!device || !device.isOnline) {
      console.log(`Device ${command.deviceId} is offline, skipping command ${command.id}`);
      return;
    }

    console.log(`Executing command ${command.id}: ${command.type} on device ${command.deviceId}`);
    
    this.stateManager.updateCommand(command.id, {
      status: 'executing',
      attempts: command.attempts + 1
    });

    try {
      const result = await this.sendCommandToDevice(device.ipAddress, command);
      
      if (result.success) {
        this.stateManager.updateCommand(command.id, {
          status: 'completed',
          executedAt: new Date()
        });
        
        // Update device state based on command type
        this.updateDeviceStateAfterCommand(command, device);
        
        console.log(`Command ${command.id} completed successfully`);
      } else {
        await this.handleCommandFailure(command, result.error || 'Unknown error');
      }
    } catch (error) {
      await this.handleCommandFailure(command, error instanceof Error ? error.message : 'Unknown error');
    }
  }

  private async sendCommandToDevice(deviceIp: string, command: Command): Promise<CommandResult> {
    const url = this.buildDeviceUrl(deviceIp, command);
    const timestamp = new Date();

    try {
      const controller = new AbortController();
      const timeoutId = setTimeout(() => controller.abort(), 10000); // 10 second timeout

      const response = await fetch(url, {
        method: 'POST',
        signal: controller.signal,
        headers: {
          'Content-Type': 'application/json',
        },
        body: command.payload ? JSON.stringify(command.payload) : undefined
      });

      clearTimeout(timeoutId);

      if (response.ok) {
        return { success: true, timestamp };
      } else {
        return { 
          success: false, 
          error: `HTTP ${response.status}: ${response.statusText}`,
          timestamp 
        };
      }
    } catch (error) {
      if (error instanceof Error && error.name === 'AbortError') {
        return { success: false, error: 'Request timeout', timestamp };
      }
      return { 
        success: false, 
        error: error instanceof Error ? error.message : 'Network error',
        timestamp 
      };
    }
  }

  private buildDeviceUrl(deviceIp: string, command: Command): string {
    const baseUrl = `http://${deviceIp}`;
    
    switch (command.type) {
      case 'output-on':
      case 'valve-open':
        return `${baseUrl}/output-on`;
      case 'output-off':
      case 'valve-close':
        return `${baseUrl}/output-off`;
      case 'one-sec-on':
        // For one-sec-on, we'll send output-on and schedule output-off
        return `${baseUrl}/output-on`;
      default:
        throw new Error(`Unsupported command type: ${command.type}`);
    }
  }

  private updateDeviceStateAfterCommand(command: Command, device: any): void {
    switch (command.type) {
      case 'output-on':
      case 'valve-open':
        this.stateManager.updateDeviceOutput(device.id, true);
        break;
      case 'output-off':
      case 'valve-close':
        this.stateManager.updateDeviceOutput(device.id, false);
        break;
      case 'one-sec-on':
        // Schedule an output-off command for 1 second later
        this.stateManager.updateDeviceOutput(device.id, true);
        this.queueScheduledCommand(
          device.id,
          'output-off',
          new Date(Date.now() + 1000)
        );
        break;
    }
  }

  private async handleCommandFailure(command: Command, error: string): Promise<void> {
    console.error(`Command ${command.id} failed (attempt ${command.attempts}): ${error}`);
    
    if (command.attempts >= command.maxAttempts) {
      this.stateManager.updateCommand(command.id, {
        status: 'failed',
        error
      });
      console.error(`Command ${command.id} failed permanently after ${command.attempts} attempts`);
    } else {
      // Reset to pending for retry
      this.stateManager.updateCommand(command.id, {
        status: 'pending',
        error
      });
      console.log(`Command ${command.id} will be retried (attempt ${command.attempts}/${command.maxAttempts})`);
    }
  }

  private generateCommandId(): string {
    return `cmd_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  }

  // Public methods for external control
  cancelCommand(commandId: string): boolean {
    const command = this.stateManager.getCommand(commandId);
    if (!command || command.status !== 'pending') {
      return false;
    }

    this.stateManager.updateCommand(commandId, {
      status: 'cancelled'
    });

    return true;
  }

  retryCommand(commandId: string): boolean {
    const command = this.stateManager.getCommand(commandId);
    if (!command || command.status !== 'failed') {
      return false;
    }

    this.stateManager.updateCommand(commandId, {
      status: 'pending',
      attempts: 0,
      error: undefined
    });

    return true;
  }

  getQueueStats() {
    const commands = this.stateManager.getAllCommands();
    
    return {
      pending: commands.filter(c => c.status === 'pending').length,
      executing: commands.filter(c => c.status === 'executing').length,
      completed: commands.filter(c => c.status === 'completed').length,
      failed: commands.filter(c => c.status === 'failed').length,
      total: commands.length
    };
  }
}