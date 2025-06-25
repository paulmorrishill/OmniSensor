/// <reference lib="deno.unstable" />

import { DeviceManager } from "./DeviceManager.ts";
import {
  DeviceConfiguration,
  ConfigureDeviceRequest,
  compareConfigurations,
  isValidDeviceMode
} from "../types/deviceConfig.ts";

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
  wifi: {
    ssid: string;
    password: string;
  };
}

// Simple SSDP client implementation
class SSSDPClient {
  private socket?: Deno.DatagramConn;
  private onDevice?: (device: any) => void;
  private isListening = false;

  constructor(onDevice: (device: any) => void) {
    this.onDevice = onDevice;
  }

  // Public method to trigger device discovery callback
  public triggerDeviceFound(device: any): void {
    if (this.onDevice) {
      this.onDevice(device);
    }
  }

  async start(): Promise<void> {
    try {
      // Create UDP socket bound to SSDP multicast port for receiving responses
      // Use a random high port to avoid conflicts
      const randomPort = 49152 + Math.floor(Math.random() * 16384); // Use ephemeral port range
      
      this.socket = Deno.listenDatagram({
        port: randomPort,
        transport: "udp",
        hostname: "0.0.0.0"
      });

      // Start listening for responses
      this.listen();
      
      console.log(`🔍 SSDP client started on port ${randomPort}`);
    } catch (error) {
      console.error('❌ Failed to start SSDP client:', error);
      throw error;
    }
  }

  async search(searchTarget: string = "upnp:rootdevice"): Promise<void> {
    if (!this.socket) {
      throw new Error("SSDP client not started");
    }

    const localAddr = this.socket.addr as Deno.NetAddr;
    
    const msearchMessage = [
      'M-SEARCH * HTTP/1.1',
      'HOST: 239.255.255.250:1900',
      'MAN: "ssdp:discover"',
      `ST: ${searchTarget}`,
      'MX: 3',
      `USER-AGENT: Deno/1.0 UPnP/1.0 OmniSensor/1.0`,
      '',
      ''
    ].join('\r\n');

    const encoder = new TextEncoder();
    const data = encoder.encode(msearchMessage);

    try {
      // Send M-SEARCH from the same socket we're listening on
      await this.socket.send(data, {
        transport: "udp",
        hostname: "239.255.255.250",
        port: 1900
      });
      
      console.log(`🔍 SSDP M-SEARCH sent from port ${localAddr.port} to 239.255.255.250:1900`);
    } catch (error) {
      console.error('❌ Failed to send SSDP M-SEARCH:', error);
      throw error;
    }
  }

  private async listen(): Promise<void> {
    if (!this.socket || this.isListening) return;
    
    this.isListening = true;
    console.log('🔍 Starting SSDP response listener...');

    try {
      for await (const [data, addr] of this.socket) {
        const message = new TextDecoder().decode(data);
        //console.log(`📨 Received UDP packet from ${(addr as Deno.NetAddr).hostname}:${(addr as Deno.NetAddr).port}`);
        //console.log(`📨 Message preview: ${message.substring(0, 100)}...`);
        this.handleResponse(message, addr);
      }
    } catch (error: unknown) {
      if (error instanceof Error && !error.message.includes('closed')) {
        console.error('❌ Error listening for SSDP responses:', error);
      }
    } finally {
      this.isListening = false;
    }
  }

  private handleResponse(message: string, addr: Deno.Addr): void {
    try {
      const netAddr = addr as Deno.NetAddr;
      const ip = netAddr.hostname;
      
      //console.log(`🔍 Processing response from ${ip}:${netAddr.port}`);
      
      // Handle both SSDP responses and NOTIFY messages
      if (!message.startsWith('HTTP/1.1 200 OK') && !message.startsWith('NOTIFY')) {
        //console.log(`⚠️ Ignoring non-SSDP message from ${ip}: ${message.substring(0, 50)}...`);
        return;
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

      /*console.log(`🔍 SSDP headers from ${ip}:`, {
        server: headers['server'],
        st: headers['st'],
        usn: headers['usn'],
        location: headers['location'],
        nt: headers['nt']
      });*/

      // Check if this looks like our device - be more permissive
      const isOurDevice = (
        headers['server']?.includes('WiFi Omni')
      );

      if (isOurDevice) {
        // console.log(`✅ Found potential device at ${ip}`);
        
        if (this.onDevice) {
          this.onDevice({
            ip,
            headers,
            timestamp: new Date()
          });
        }
      } else {
        //console.log(`⚠️ Device at ${ip} doesn't match our criteria`);
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
      
      // Try multiple search targets
      await this.ssdpClient.search("upnp:rootdevice");
      await new Promise(resolve => setTimeout(resolve, 500)); // Small delay between searches
      
      await this.ssdpClient.search("urn:schemas-upnp-org:device:Basic:1");
      await new Promise(resolve => setTimeout(resolve, 500));
      
      await this.ssdpClient.search("ssdp:all");
      await new Promise(resolve => setTimeout(resolve, 500));
      
      // Also try a direct network scan as fallback
      //await this.performNetworkScan();
      
    } catch (error) {
      console.error('❌ Error during device discovery:', error);
    }
  }

  private async performNetworkScan(): Promise<void> {
    console.log('🔍 Performing direct network scan as fallback...');
    
    // Get our server IP to determine network range
    const serverIP = this.config.server.ip;
    const ipParts = serverIP.split('.');
    
    if (ipParts.length !== 4) {
      console.log('⚠️ Invalid server IP format, skipping network scan');
      return;
    }
    
    const networkBase = `${ipParts[0]}.${ipParts[1]}.${ipParts[2]}`;
    const promises: Promise<void>[] = [];
    
    // Scan common IP ranges (last octet 1-254)
    for (let i = 1; i <= 254; i++) {
      const ip = `${networkBase}.${i}`;
      
      // Skip our own IP
      if (ip === serverIP) continue;
      
      promises.push(this.checkDeviceDirectly(ip));
      
      // Limit concurrent requests to avoid overwhelming the network
      if (promises.length >= 20) {
        await Promise.allSettled(promises);
        promises.length = 0;
        await new Promise(resolve => setTimeout(resolve, 100)); // Small delay
      }
    }
    
    // Process remaining promises
    if (promises.length > 0) {
      await Promise.allSettled(promises);
    }
  }

  private async checkDeviceDirectly(ip: string): Promise<void> {
    try {
      // Quick check with very short timeout
      const response = await fetch(`http://${ip}/is-up`, {
        signal: AbortSignal.timeout(1000)
      });
      
      if (response.ok && (await response.text()).trim() === 'yes') {
        console.log(`🔍 Found device via direct scan: ${ip}`);
        
        // Simulate SSDP discovery for this device
        if (this.ssdpClient) {
          this.ssdpClient.triggerDeviceFound({
            ip,
            headers: {
              server: 'Direct Scan Discovery',
              location: `http://${ip}/description.xml`
            },
            timestamp: new Date()
          });
        }
      }
    } catch (error) {
      // Silently ignore errors for direct scan - most IPs won't respond
    }
  }

  private async handleDiscoveredDevice(ssdpDevice: any): Promise<void> {
    const ip = ssdpDevice.ip;
    
    // Skip our own server IP
    if (ip === this.config.server.ip) {
      return;
    }

    try {
      // console.log(`🔍 Checking device at ${ip}...`);

      // Verify it's our device by checking the /is-up endpoint
      const isUpResponse = await fetch(`http://${ip}/is-up`, {
        signal: AbortSignal.timeout(2000)
      });

      if (!isUpResponse.ok || (await isUpResponse.text()).trim() !== 'yes') {
        return; // Not our device
      }

      // Get device configuration using the new API
      const configResponse = await fetch(`http://${ip}/api/config`, {
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
      
      // Prepare the desired configuration
      const desiredConfig: DeviceConfiguration = {
        ssid: this.config.wifi.ssid,
        alias: currentConfig.alias || device.hostname || `Device_${ip.split('.').pop()}`,
        server: serverUrl,
        mode: currentConfig.mode ?? this.config.device.defaultMode
      };
      
      // Validate the mode
      if (!isValidDeviceMode(desiredConfig.mode)) {
        console.error(`❌ Invalid device mode: ${desiredConfig.mode}, using default mode ${this.config.device.defaultMode}`);
        desiredConfig.mode = this.config.device.defaultMode;
      }
      
      // Get current configuration for comparison
      const currentDeviceConfig: DeviceConfiguration = {
        ssid: currentConfig.storedSsid || '',
        alias: currentConfig.alias || '',
        server: currentConfig.server || '',
        mode: currentConfig.mode ?? 0
      };
      
      // Compare configurations to see if update is needed
      const comparison = compareConfigurations(currentDeviceConfig, desiredConfig);
      
      if (!comparison.hasChanges) {
        console.log(`✅ Device at ${ip} already has correct configuration - no update needed`);
        device.isConfigured = true;
        this.discoveredDevices.set(ip, device);
        return;
      }
      
      // Log what will be changed
      const changes: string[] = [];
      if (!comparison.ssid) changes.push(`SSID: "${currentDeviceConfig.ssid}" → "${desiredConfig.ssid}"`);
      if (!comparison.alias) changes.push(`Alias: "${currentDeviceConfig.alias}" → "${desiredConfig.alias}"`);
      if (!comparison.server) changes.push(`Server: "${currentDeviceConfig.server}" → "${desiredConfig.server}"`);
      if (!comparison.mode) changes.push(`Mode: ${currentDeviceConfig.mode} → ${desiredConfig.mode}`);
      
      console.log(`🔧 Configuration changes needed for device at ${ip}:`);
      changes.forEach(change => console.log(`   - ${change}`));
      
      // Prepare JSON configuration data for the new API
      const configData = {
        ssid: desiredConfig.ssid,
        password: this.config.wifi.password,
        alias: desiredConfig.alias,
        server: desiredConfig.server,
        mode: desiredConfig.mode
      };
      
      console.log(`🔧 Sending configuration update to device at ${ip}...`);
      
      // Send configuration using the new JSON API
      const response = await fetch(`http://${ip}/api/config`, {
        method: 'POST',
        headers: {
          'Content-Type': 'application/json'
        },
        body: JSON.stringify(configData),
        signal: AbortSignal.timeout(this.config.device.configTimeout)
      });
      
      if (response.ok) {
        console.log(`✅ Successfully configured device at ${ip} with server ${serverUrl}`);
        device.isConfigured = true;
        this.discoveredDevices.set(ip, device);
      } else {
        const responseText = await response.text().catch(() => 'Unknown error');
        console.error(`❌ Failed to configure device at ${ip}: ${response.status} - ${responseText}`);
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
      console.log(`🔧 Manual configuration requested for device at ${ip}`);
      
      const configResponse = await fetch(`http://${ip}/api/config`, {
        signal: AbortSignal.timeout(3000)
      });
      
      if (!configResponse.ok) {
        console.error(`❌ Failed to get current config from device at ${ip}: ${configResponse.status}`);
        return false;
      }
      
      const currentConfig = await configResponse.json();
      
      await this.configureDevice(ip, device, currentConfig);
      return device.isConfigured;
    } catch (error) {
      console.error(`❌ Failed to manually configure device at ${ip}:`, error);
      return false;
    }
  }

  // Force a new discovery scan
  async forceDiscovery(): Promise<void> {
    console.log('🔍 Forcing device discovery...');
    await this.discoverDevices();
  }
}
