/// <reference lib="deno.unstable" />

import { DeviceManager } from "./DeviceManager.ts";

export interface DiscoveredDevice {
  ip: string;
  hostname?: string;
  deviceId?: string;
  serialNumber?: string;
  modelName?: string;
  isConfigured: boolean;
  lastSeen: Date;
  ssdpInfo?: any;
}

export interface ServerConfig {
  server: {
    port: number;
    host: string;
    ip: string;
  };
  discovery: {
    enabled: boolean;
    scanInterval: number;
    autoConfigureDevices: boolean;
    ssdpPort: number;
  };
  device: {
    defaultMode: number;
    configTimeout: number;
  };
}

// Simple SSDP client implementation
class SSSDPClient {
  private socket?: Deno.DatagramConn;
  private onDevice?: (device: any) => void;

  constructor(onDevice: (device: any) => void) {
    this.onDevice = onDevice;
  }

  async start(): Promise<void> {
    try {
      // Create UDP socket for receiving SSDP responses
      this.socket = Deno.listenDatagram({
        port: 0, // Let system assign port
        transport: "udp",
        hostname: "0.0.0.0"
      });

      // Start listening for responses
      this.listen();
      
      console.log('🔍 SSDP client started');
    } catch (error) {
      console.error('❌ Failed to start SSDP client:', error);
      throw error;
    }
  }

  async search(searchTarget: string = "upnp:rootdevice"): Promise<void> {
    const msearchMessage = [
      'M-SEARCH * HTTP/1.1',
      'HOST: 239.255.255.250:1900',
      'MAN: "ssdp:discover"',
      `ST: ${searchTarget}`,
      'MX: 3',
      '',
      ''
    ].join('\r\n');

    const encoder = new TextEncoder();
    const data = encoder.encode(msearchMessage);

    // Create a separate socket for sending
    const sendSocket = Deno.listenDatagram({
      port: 0,
      transport: "udp"
    });

    try {
      await sendSocket.send(data, {
        transport: "udp",
        hostname: "239.255.255.250",
        port: 1900
      });
      
      console.log('🔍 SSDP M-SEARCH sent');
    } finally {
      sendSocket.close();
    }
  }

  private async listen(): Promise<void> {
    if (!this.socket) return;

    try {
      for await (const [data, addr] of this.socket) {
        const message = new TextDecoder().decode(data);
        this.handleResponse(message, addr);
      }
    } catch (error: unknown) {
      if (error instanceof Error && !error.message.includes('closed')) {
        console.error('❌ Error listening for SSDP responses:', error);
      }
    }
  }

  private handleResponse(message: string, addr: Deno.Addr): void {
    try {
      if (!message.startsWith('HTTP/1.1 200 OK')) {
        return; // Not a valid SSDP response
      }

      const lines = message.split('\r\n');
      const headers: Record<string, string> = {};
      
      for (const line of lines) {
        const colonIndex = line.indexOf(':');
        if (colonIndex > 0) {
          const key = line.substring(0, colonIndex).trim().toLowerCase();
          const value = line.substring(colonIndex + 1).trim();
          headers[key] = value;
        }
      }

      // Check if this looks like our device
      if (headers['server'] && (
          headers['server'].includes('WiFi Omni') ||
          headers['server'].includes('ESP') ||
          headers['usn']?.includes('WiFi')
        )) {
        
        const ip = (addr as { hostname: string }).hostname;
        
        if (this.onDevice) {
          this.onDevice({
            ip,
            headers,
            timestamp: new Date()
          });
        }
      }
    } catch (error) {
      console.error('❌ Error handling SSDP response:', error);
    }
  }

  stop(): void {
    if (this.socket) {
      this.socket.close();
      this.socket = undefined;
    }
  }
}

export class NetworkDiscovery {
  private deviceManager: DeviceManager;
  private discoveredDevices: Map<string, DiscoveredDevice> = new Map();
  private intervalId?: number;
  private config: ServerConfig;
  private ssdpClient?: SSSDPClient;

  constructor(deviceManager: DeviceManager, config: ServerConfig) {
    this.deviceManager = deviceManager;
    this.config = config;
  }

  async start(): Promise<void> {
    if (!this.config.discovery.enabled) {
      console.log('🔍 Network discovery is disabled in configuration');
      return;
    }

    if (this.intervalId) return;
    
    console.log('🔍 Starting SSDP device discovery...');
    console.log(`🔍 Server IP: ${this.config.server.ip}:${this.config.server.port}`);
    
    try {
      // Initialize SSDP client
      this.ssdpClient = new SSSDPClient((device) => {
        this.handleDiscoveredDevice(device);
      });
      
      await this.ssdpClient.start();
      
      // Initial discovery
      await this.discoverDevices();
      
      // Schedule periodic discovery
      this.intervalId = setInterval(async () => {
        await this.discoverDevices();
        this.cleanupOldDevices();
      }, this.config.discovery.scanInterval);
      
      console.log('🔍 SSDP discovery started successfully');
    } catch (error) {
      console.error('❌ Failed to start SSDP discovery:', error);
    }
  }

  async stop(): Promise<void> {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = undefined;
    }
    
    if (this.ssdpClient) {
      this.ssdpClient.stop();
      this.ssdpClient = undefined;
    }
    
    console.log('🔍 SSDP discovery stopped');
  }

  private async discoverDevices(): Promise<void> {
    if (!this.ssdpClient) return;
    
    try {
      console.log('🔍 Searching for devices...');
      await this.ssdpClient.search("upnp:rootdevice");
      
      // Also search for our specific device type
      await this.ssdpClient.search("urn:schemas-upnp-org:device:Basic:1");
      
    } catch (error) {
      console.error('❌ Error during device discovery:', error);
    }
  }

  private async handleDiscoveredDevice(ssdpDevice: any): Promise<void> {
    const ip = ssdpDevice.ip;
    
    // Skip our own server IP
    if (ip === this.config.server.ip) {
      return;
    }

    try {
      console.log(`🔍 Checking device at ${ip}...`);

      // Verify it's our device by checking the /is-up endpoint
      const isUpResponse = await fetch(`http://${ip}/is-up`, {
        signal: AbortSignal.timeout(2000)
      });

      if (!isUpResponse.ok || (await isUpResponse.text()).trim() !== 'yes') {
        return; // Not our device
      }

      // Get device configuration
      const configResponse = await fetch(`http://${ip}/currentConfig`, {
        signal: AbortSignal.timeout(3000)
      });
      
      if (!configResponse.ok) {
        return;
      }

      const currentConfig = await configResponse.json();
      
      const device: DiscoveredDevice = {
        ip,
        hostname: currentConfig.alias || 'Unknown Device',
        deviceId: currentConfig.deviceId,
        serialNumber: ssdpDevice.headers.usn,
        modelName: 'WiFi Omni',
        isConfigured: !!currentConfig.server && this.isOurServer(currentConfig.server),
        lastSeen: new Date(),
        ssdpInfo: ssdpDevice.headers
      };

      this.discoveredDevices.set(ip, device);

      // If device is not configured with our server, configure it
      if (this.config.discovery.autoConfigureDevices && !device.isConfigured) {
        await this.configureDevice(ip, device, currentConfig);
      }

      console.log(`🔍 Found WiFi device: ${device.hostname} at ${ip} (configured: ${device.isConfigured})`);

    } catch (error) {
      console.error(`❌ Error processing device at ${ip}:`, error);
    }
  }

  private isOurServer(serverUrl: string): boolean {
    try {
      const url = new URL(serverUrl);
      
      // Check if the server URL points to our server
      return (url.hostname === this.config.server.ip || 
              url.hostname === 'localhost' || 
              url.hostname === '127.0.0.1') &&
             url.port === this.config.server.port.toString();
    } catch (error) {
      return false;
    }
  }

  private async configureDevice(ip: string, device: DiscoveredDevice, currentConfig: any): Promise<void> {
    try {
      console.log(`🔧 Auto-configuring device at ${ip}...`);
      
      const serverUrl = `http://${this.config.server.ip}:${this.config.server.port}`;
      
      // Prepare configuration data
      const configData = new URLSearchParams({
        ssid: currentConfig.storedSsid || '',
        password: '', // Keep existing password
        alias: currentConfig.alias || device.hostname || `Device_${ip.split('.').pop()}`,
        server: serverUrl,
        mode: currentConfig.mode?.toString() || this.config.device.defaultMode.toString()
      });
      
      // Send configuration
      const response = await fetch(`http://${ip}/configure`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/x-www-form-urlencoded'
        },
        body: configData.toString(),
        signal: AbortSignal.timeout(this.config.device.configTimeout)
      });
      
      if (response.ok) {
        console.log(`✅ Successfully configured device at ${ip} with server ${serverUrl}`);
        device.isConfigured = true;
        this.discoveredDevices.set(ip, device);
      } else {
        console.error(`❌ Failed to configure device at ${ip}: ${response.status}`);
      }
      
    } catch (error) {
      console.error(`❌ Error configuring device at ${ip}:`, error);
    }
  }

  private cleanupOldDevices(): void {
    const now = new Date();
    const maxAge = 5 * 60 * 1000; // 5 minutes
    
    for (const [ip, device] of this.discoveredDevices.entries()) {
      if (now.getTime() - device.lastSeen.getTime() > maxAge) {
        this.discoveredDevices.delete(ip);
        console.log(`🗑️ Removed stale device: ${ip}`);
      }
    }
  }

  getDiscoveredDevices(): DiscoveredDevice[] {
    return Array.from(this.discoveredDevices.values());
  }

  getDeviceByIP(ip: string): DiscoveredDevice | undefined {
    return this.discoveredDevices.get(ip);
  }

  // Manual device configuration trigger
  async configureDeviceManually(ip: string): Promise<boolean> {
    const device = this.discoveredDevices.get(ip);
    if (!device) {
      console.error(`Device at ${ip} not found in discovered devices`);
      return false;
    }

    try {
      const configResponse = await fetch(`http://${ip}/currentConfig`);
      const currentConfig = await configResponse.json();
      
      await this.configureDevice(ip, device, currentConfig);
      return device.isConfigured;
    } catch (error) {
      console.error(`Failed to manually configure device at ${ip}:`, error);
      return false;
    }
  }

  // Force a new discovery scan
  async forceDiscovery(): Promise<void> {
    console.log('🔍 Forcing device discovery...');
    await this.discoverDevices();
  }
}