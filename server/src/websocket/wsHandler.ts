import { StateManager } from "../managers/StateManager.ts";
import { StateMessage } from "../types/api.ts";

export class WebSocketHandler {
  private stateManager: StateManager;
  private connections: Set<WebSocket> = new Set();

  constructor(stateManager: StateManager) {
    this.stateManager = stateManager;
    
    // Listen for state changes and broadcast to all connected clients
    this.stateManager.addListener((state) => {
      this.broadcastState(state);
    });
  }

  handleConnection(ws: WebSocket): void {
    console.log('New WebSocket connection established');
    
    this.connections.add(ws);

    // Send current state immediately upon connection
    this.sendStateToClient(ws, this.stateManager.getState());

    ws.addEventListener('close', () => {
      console.log('WebSocket connection closed');
      this.connections.delete(ws);
    });

    ws.addEventListener('error', (event) => {
      console.error('WebSocket error:', event);
      this.connections.delete(ws);
    });

    ws.addEventListener('message', (event) => {
      try {
        const message = JSON.parse(event.data);
        this.handleMessage(ws, message);
      } catch (error) {
        console.error('Error parsing WebSocket message:', error);
        this.sendError(ws, 'Invalid JSON message');
      }
    });
  }

  private handleMessage(ws: WebSocket, message: any): void {
    switch (message.type) {
      case 'ping':
        this.sendMessage(ws, { type: 'pong', timestamp: new Date() });
        break;
      
      case 'request-state':
        this.sendStateToClient(ws, this.stateManager.getState());
        break;
      
      default:
        console.warn('Unknown WebSocket message type:', message.type);
        this.sendError(ws, `Unknown message type: ${message.type}`);
    }
  }

  private broadcastState(state: any): void {
    const message: StateMessage = {
      type: 'full-state',
      timestamp: new Date(),
      data: state
    };

    this.broadcast(message);
  }

  private sendStateToClient(ws: WebSocket, state: any): void {
    const message: StateMessage = {
      type: 'full-state',
      timestamp: new Date(),
      data: state
    };

    this.sendMessage(ws, message);
  }

  private broadcast(message: any): void {
    const messageStr = JSON.stringify(message);
    const deadConnections: WebSocket[] = [];

    for (const ws of this.connections) {
      try {
        if (ws.readyState === WebSocket.OPEN) {
          ws.send(messageStr);
        } else {
          deadConnections.push(ws);
        }
      } catch (error) {
        console.error('Error sending WebSocket message:', error);
        deadConnections.push(ws);
      }
    }

    // Clean up dead connections
    deadConnections.forEach(ws => this.connections.delete(ws));
  }

  private sendMessage(ws: WebSocket, message: any): void {
    try {
      if (ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify(message));
      }
    } catch (error) {
      console.error('Error sending WebSocket message:', error);
      this.connections.delete(ws);
    }
  }

  private sendError(ws: WebSocket, error: string): void {
    this.sendMessage(ws, {
      type: 'error',
      error,
      timestamp: new Date()
    });
  }

  getConnectionCount(): number {
    return this.connections.size;
  }

  closeAllConnections(): void {
    for (const ws of this.connections) {
      try {
        ws.close();
      } catch (error) {
        console.error('Error closing WebSocket connection:', error);
      }
    }
    this.connections.clear();
  }
}