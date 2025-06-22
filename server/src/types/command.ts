export type CommandType =
  | 'output-on'
  | 'output-off'
  | 'set-mode'
  | 'rename'
  | 'one-sec-on'
  | 'valve-open'
  | 'valve-close';

export type CommandStatus = 
  | 'pending' 
  | 'executing' 
  | 'completed' 
  | 'failed' 
  | 'cancelled';

export interface Command {
  id: string;
  deviceId: string;
  type: CommandType;
  payload?: any;
  scheduledFor: Date;
  createdAt: Date;
  attempts: number;
  maxAttempts: number;
  status: CommandStatus;
  error?: string;
  executedAt?: Date;
}

export interface CommandRequest {
  type: CommandType;
  payload?: any;
  scheduleDelay?: number; // milliseconds from now
}

export interface ScheduledCommand extends Command {
  scheduledFor: Date;
}

export interface CommandResult {
  success: boolean;
  error?: string;
  timestamp: Date;
}

export interface CommandQueueStats {
  pending: number;
  executing: number;
  completed: number;
  failed: number;
  total: number;
}