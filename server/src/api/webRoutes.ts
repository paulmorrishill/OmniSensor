import { Router, RouterContext } from "oak";
import { DeviceManager } from "../managers/DeviceManager.ts";

// Import type extensions
import "../types/oak.d.ts";

// Error handling wrapper for route handlers
function withErrorHandling(handler: (ctx: RouterContext<string>) => Promise<void>) {
  return async (ctx: RouterContext<string>) => {
    try {
      await handler(ctx);
    } catch (error) {
      console.error("Route error:", error);
      ctx.response.status = 500;
      ctx.response.body = "Internal server error";
    }
  };
}

export function createWebRoutes(deviceManager: DeviceManager): Router {
  const router = new Router();

  // Dashboard page
  router.get("/", withErrorHandling(async (ctx) => {
    const devices = deviceManager.getAllDevicesStatus();
    const systemHealth = deviceManager.getSystemHealth();
    
    await ctx.render("pages/dashboard", {
      title: "WiFi Device Dashboard",
      devices,
      systemHealth,
      currentTime: new Date().toISOString()
    });
  }));

  // Individual device page
  router.get("/device/:id", withErrorHandling(async (ctx) => {
    const deviceId = ctx.params.id;
    const device = deviceManager.getDeviceStatus(deviceId);
    
    if (!device) {
      ctx.response.status = 404;
      await ctx.render("pages/error", {
        title: "Device Not Found",
        error: `Device ${deviceId} not found`,
        backUrl: "/"
      });
      return;
    }

    await ctx.render("pages/device", {
      title: `Device: ${device.alias}`,
      device,
      currentTime: new Date().toISOString()
    });
  }));

  // Settings page
  router.get("/settings", withErrorHandling(async (ctx) => {
    const systemHealth = deviceManager.getSystemHealth();
    
    await ctx.render("pages/settings", {
      title: "System Settings",
      systemHealth,
      currentTime: new Date().toISOString()
    });
  }));

  // Discovery page
  router.get("/discovery", withErrorHandling(async (ctx) => {
    await ctx.render("pages/discovery", {
      title: "Device Discovery",
      currentTime: new Date().toISOString()
    });
  }));

  return router;
}