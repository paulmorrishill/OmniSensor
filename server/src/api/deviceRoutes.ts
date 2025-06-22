import { Router } from "oak";
import { DeviceManager } from "../managers/DeviceManager.ts";
import { DeviceRegistration, WiFiFailureReport } from "../types/device.ts";

export function createDeviceRoutes(deviceManager: DeviceManager): Router {
  const router = new Router();

  // Device registration endpoint
  router.post("/register", async (ctx) => {
    try {
      const body = await ctx.request.body({ type: "json" }).value;
      
      const registration: DeviceRegistration = {
        id: body.id,
        alias: body.alias,
        ipAddress: body.ipAddress,
        macAddress: body.macAddress,
        mode: body.mode
      };

      // Validate required fields
      if (!registration.id || !registration.alias || !registration.ipAddress || !registration.macAddress) {
        ctx.response.status = 400;
        ctx.response.body = { error: "Missing required fields" };
        return;
      }

      deviceManager.handleDeviceRegistration(registration);
      
      ctx.response.status = 200;
      ctx.response.body = { success: true };
      
    } catch (error) {
      console.error("Error in device registration:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  // Should remain awake endpoint
  router.get("/should-remain-awake", (ctx) => {
    try {
      const deviceId = ctx.request.url.searchParams.get("id");
      
      if (!deviceId) {
        ctx.response.status = 400;
        ctx.response.body = "Missing device ID";
        return;
      }

      const shouldStayAwake = deviceManager.handleShouldRemainAwake(deviceId);
      
      // Return "1" for stay awake, "0" for sleep (as expected by device firmware)
      ctx.response.status = 200;
      ctx.response.body = shouldStayAwake ? "1" : "0";
      
    } catch (error) {
      console.error("Error in should-remain-awake:", error);
      ctx.response.status = 500;
      ctx.response.body = "0"; // Default to sleep on error
    }
  });

  // WiFi failures endpoint
  router.post("/wifi-failures", async (ctx) => {
    try {
      const body = await ctx.request.body({ type: "json" }).value;
      
      const report: WiFiFailureReport = {
        id: body.id,
        alias: body.alias,
        failures: body.failures
      };

      // Validate required fields
      if (!report.id || !report.failures) {
        ctx.response.status = 400;
        ctx.response.body = { error: "Missing required fields" };
        return;
      }

      deviceManager.handleWiFiFailures(report);
      
      ctx.response.status = 200;
      ctx.response.body = { success: true };
      
    } catch (error) {
      console.error("Error in WiFi failures:", error);
      ctx.response.status = 500;
      ctx.response.body = { error: "Internal server error" };
    }
  });

  // Health check endpoint for devices
  router.get("/is-up", (ctx) => {
    ctx.response.status = 200;
    ctx.response.body = "yes";
  });

  return router;
}