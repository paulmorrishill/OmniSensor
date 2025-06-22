import { Router } from "oak";
import { DeviceManager } from "../managers/DeviceManager.ts";

export function createWebRoutes(deviceManager: DeviceManager): Router {
  const router = new Router();

  // Dashboard page
  router.get("/", async (ctx) => {
    try {
      const devices = deviceManager.getAllDevicesStatus();
      const systemHealth = deviceManager.getSystemHealth();
      
      await ctx.render("pages/dashboard", {
        title: "WiFi Device Dashboard",
        devices,
        systemHealth,
        currentTime: new Date().toISOString()
      });
    } catch (error) {
      console.error("Error rendering dashboard:", error);
      ctx.response.status = 500;
      ctx.response.body = "Internal server error";
    }
  });

  // Individual device page
  router.get("/device/:id", async (ctx) => {
    try {
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
    } catch (error) {
      console.error("Error rendering device page:", error);
      ctx.response.status = 500;
      ctx.response.body = "Internal server error";
    }
  });

  // Settings page
  router.get("/settings", async (ctx) => {
    try {
      const systemHealth = deviceManager.getSystemHealth();
      
      await ctx.render("pages/settings", {
        title: "System Settings",
        systemHealth,
        currentTime: new Date().toISOString()
      });
    } catch (error) {
      console.error("Error rendering settings page:", error);
      ctx.response.status = 500;
      ctx.response.body = "Internal server error";
    }
  });

  return router;
}