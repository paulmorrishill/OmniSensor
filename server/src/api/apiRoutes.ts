import { Router } from "oak";
import { DeviceManager } from "../managers/DeviceManager.ts";
import { DeviceControlRequest, DeviceRenameRequest } from "../types/api.ts";
import { createApiResponse, createErrorResponse } from "../middleware/errorHandler.ts";

export function createApiRoutes(deviceManager: DeviceManager): Router {
  const router = new Router();

  // Get all devices
  router.get("/api/devices", (ctx) => {
    const devices = deviceManager.getAllDevicesStatus();
    ctx.response.body = createApiResponse(devices);
  });

  // Get specific device
  router.get("/api/devices/:id", (ctx) => {
    const deviceId = ctx.params.id;
    const device = deviceManager.getDeviceStatus(deviceId);
    
    if (!device) {
      const { status, response } = createErrorResponse("Device not found", 404);
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }

    ctx.response.body = createApiResponse(device);
  });

  // Control device
  router.post("/api/devices/:id/control", async (ctx) => {
    const deviceId = ctx.params.id;
    const body: DeviceControlRequest = await ctx.request.body({ type: "json" }).value;
    
    if (!body.action) {
      const { status, response } = createErrorResponse("Missing action parameter");
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }

    const commandId = deviceManager.controlDevice(deviceId, body.action, body.scheduleDelay);
    
    if (!commandId) {
      const { status, response } = createErrorResponse("Device not found", 404);
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }

    ctx.response.body = createApiResponse({ commandId });
  });

  // Rename device
  router.post("/api/devices/:id/rename", async (ctx) => {
    const deviceId = ctx.params.id;
    const body: DeviceRenameRequest = await ctx.request.body({ type: "json" }).value;
    
    if (!body.alias || body.alias.trim().length === 0) {
      const { status, response } = createErrorResponse("Missing or empty alias");
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }

    const success = deviceManager.renameDevice(deviceId, body.alias.trim());
    
    if (!success) {
      const { status, response } = createErrorResponse("Device not found", 404);
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }

    ctx.response.body = createApiResponse({ alias: body.alias.trim() });
  });

  // Get system health
  router.get("/api/health", (ctx) => {
    const health = deviceManager.getSystemHealth();
    ctx.response.body = createApiResponse(health);
  });

  // Bulk operations
  router.post("/api/devices/wake-all", (ctx) => {
    const commandIds = deviceManager.wakeAllDevices();
    ctx.response.body = createApiResponse({ commandIds, count: commandIds.length });
  });

  router.post("/api/devices/sleep-all", (ctx) => {
    const commandIds = deviceManager.sleepAllDevices();
    ctx.response.body = createApiResponse({ commandIds, count: commandIds.length });
  });

  // Command management
  router.post("/api/commands/:id/cancel", (ctx) => {
    const commandId = ctx.params.id;
    const success = deviceManager.cancelCommand(commandId);
    
    if (!success) {
      const { status, response } = createErrorResponse("Command not found or cannot be cancelled", 404);
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }
    
    ctx.response.body = createApiResponse({ cancelled: true });
  });

  router.post("/api/commands/:id/retry", (ctx) => {
    const commandId = ctx.params.id;
    const success = deviceManager.retryCommand(commandId);
    
    if (!success) {
      const { status, response } = createErrorResponse("Command not found or cannot be retried", 404);
      ctx.response.status = status;
      ctx.response.body = response;
      return;
    }
    
    ctx.response.body = createApiResponse({ retried: true });
  });

  return router;
}