import { Router } from "oak";
import { NetworkDiscovery } from "../managers/NetworkDiscovery.ts";

export function createDiscoveryRoutes(networkDiscovery: NetworkDiscovery): Router {
  const router = new Router();

  // Get all discovered devices
  router.get("/api/discovery/devices", (ctx) => {
    try {
      const devices = networkDiscovery.getDiscoveredDevices();
      
      ctx.response.status = 200;
      ctx.response.body = {
        success: true,
        devices: devices.map(device => ({
          ip: device.ip,
          hostname: device.hostname,
          deviceId: device.deviceId,
          serialNumber: device.serialNumber,
          modelName: device.modelName,
          isConfigured: device.isConfigured,
          lastSeen: device.lastSeen.toISOString(),
          timeSinceLastSeen: Date.now() - device.lastSeen.getTime()
        }))
      };
      
    } catch (error) {
      console.error("Error getting discovered devices:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  // Get specific device by IP
  router.get("/api/discovery/devices/:ip", (ctx) => {
    try {
      const ip = ctx.params.ip;
      const device = networkDiscovery.getDeviceByIP(ip);
      
      if (!device) {
        ctx.response.status = 404;
        ctx.response.body = { error: "Device not found" };
        return;
      }

      ctx.response.status = 200;
      ctx.response.body = {
        success: true,
        device: {
          ip: device.ip,
          hostname: device.hostname,
          deviceId: device.deviceId,
          serialNumber: device.serialNumber,
          modelName: device.modelName,
          isConfigured: device.isConfigured,
          lastSeen: device.lastSeen.toISOString(),
          timeSinceLastSeen: Date.now() - device.lastSeen.getTime(),
          ssdpInfo: device.ssdpInfo
        }
      };
      
    } catch (error) {
      console.error("Error getting device:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  // Force discovery scan
  router.post("/api/discovery/scan", async (ctx) => {
    try {
      await networkDiscovery.forceDiscovery();
      
      ctx.response.status = 200;
      ctx.response.body = {
        success: true,
        message: "Discovery scan initiated"
      };
      
    } catch (error) {
      console.error("Error forcing discovery:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  // Configure specific device
  router.post("/api/discovery/configure/:ip", async (ctx) => {
    try {
      const ip = ctx.params.ip;
      const success = await networkDiscovery.configureDeviceManually(ip);
      
      if (success) {
        ctx.response.status = 200;
        ctx.response.body = {
          success: true,
          message: `Device at ${ip} configured successfully`
        };
      } else {
        ctx.response.status = 400;
        ctx.response.body = {
          success: false,
          error: `Failed to configure device at ${ip}`
        };
      }
      
    } catch (error) {
      console.error("Error configuring device:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  // Get discovery statistics
  router.get("/api/discovery/stats", (ctx) => {
    try {
      const devices = networkDiscovery.getDiscoveredDevices();
      const configuredDevices = devices.filter(d => d.isConfigured);
      const unconfiguredDevices = devices.filter(d => !d.isConfigured);
      
      ctx.response.status = 200;
      ctx.response.body = {
        success: true,
        stats: {
          totalDevices: devices.length,
          configuredDevices: configuredDevices.length,
          unconfiguredDevices: unconfiguredDevices.length,
          lastScanTime: devices.length > 0 ? 
            Math.max(...devices.map(d => d.lastSeen.getTime())) : null
        }
      };
      
    } catch (error) {
      console.error("Error getting discovery stats:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  return router;
}