import { Application } from "oak";
import { oakCors } from "https://deno.land/x/cors/mod.ts";
import { Eta } from "eta";

// Import type extensions
import "./src/types/oak.d.ts";

import { StateManager } from "./src/managers/StateManager.ts";
import { CommandQueue } from "./src/managers/CommandQueue.ts";
import { DeviceManager } from "./src/managers/DeviceManager.ts";
import { NetworkDiscovery, ServerConfig } from "./src/managers/NetworkDiscovery.ts";
import { WebSocketHandler } from "./src/websocket/wsHandler.ts";

import { createDeviceRoutes } from "./src/api/deviceRoutes.ts";
import { createWebRoutes } from "./src/api/webRoutes.ts";
import { createApiRoutes } from "./src/api/apiRoutes.ts";
import { createDiscoveryRoutes } from "./src/api/discoveryRoutes.ts";
import { errorHandler } from "./src/middleware/errorHandler.ts";

// Load configuration
const configText = await Deno.readTextFile("./config.json");
const config: ServerConfig = JSON.parse(configText);

const PORT = parseInt(Deno.env.get("PORT") || config.server.port.toString());

class WiFiDeviceServer {
  private app: Application;
  private stateManager: StateManager;
  private commandQueue: CommandQueue;
  private deviceManager: DeviceManager;
  private networkDiscovery: NetworkDiscovery;
  private wsHandler: WebSocketHandler;

  constructor() {
    this.app = new Application();
    this.stateManager = new StateManager();
    this.commandQueue = new CommandQueue(this.stateManager);
    this.deviceManager = new DeviceManager(this.stateManager, this.commandQueue);
    this.networkDiscovery = new NetworkDiscovery(this.deviceManager, config);
    this.wsHandler = new WebSocketHandler(this.stateManager);
    
    this.setupMiddleware();
    this.setupRoutes();
  }

  private setupMiddleware(): void {
    // CORS
    this.app.use(oakCors({
      origin: "*",
      methods: ["GET", "POST", "PUT", "DELETE"],
      allowedHeaders: ["Content-Type", "Authorization"]
    }));

    // Error handling
    this.app.use(errorHandler);

    // Static files
    this.app.use(async (ctx, next) => {
      if (ctx.request.url.pathname.startsWith("/static/")) {
        try {
          await ctx.send({
            root: `${Deno.cwd()}/static`,
            path: ctx.request.url.pathname.replace("/static", "")
          });
        } catch {
          await next();
        }
      } else {
        await next();
      }
    });

    // ETA template engine setup
    const eta = new Eta({ views: `${Deno.cwd()}/views`, cache: false });
    
    // Add render method to context
    this.app.use(async (ctx, next) => {
      ctx.render = async (template: string, data: any = {}) => {
        const html = await eta.render(template, data);
        ctx.response.type = "text/html";
        ctx.response.body = html;
      };
      await next();
    });
  }

  private setupRoutes(): void {
    // WebSocket endpoint
    this.app.use(async (ctx, next) => {
      if (ctx.request.url.pathname === "/ws") {
        if (ctx.isUpgradable) {
          const ws = ctx.upgrade();
          this.wsHandler.handleConnection(ws);
        } else {
          ctx.response.status = 400;
          ctx.response.body = "WebSocket upgrade required";
        }
      } else {
        await next();
      }
    });

    // Device communication routes
    const deviceRoutes = createDeviceRoutes(this.deviceManager);
    this.app.use(deviceRoutes.routes());
    this.app.use(deviceRoutes.allowedMethods());

    // API routes
    const apiRoutes = createApiRoutes(this.deviceManager);
    this.app.use(apiRoutes.routes());
    this.app.use(apiRoutes.allowedMethods());

    // Discovery routes
    const discoveryRoutes = createDiscoveryRoutes(this.networkDiscovery);
    this.app.use(discoveryRoutes.routes());
    this.app.use(discoveryRoutes.allowedMethods());

    // Web interface routes
    const webRoutes = createWebRoutes(this.deviceManager);
    this.app.use(webRoutes.routes());
    this.app.use(webRoutes.allowedMethods());
  }

  async start(): Promise<void> {
    // Start managers
    this.commandQueue.start();
    this.deviceManager.start();
    await this.networkDiscovery.start();

    console.log(`🚀 WiFi Device Management Server starting on port ${PORT}`);
    console.log(`📊 Dashboard: http://localhost:${PORT}`);
    console.log(`🔌 WebSocket: ws://localhost:${PORT}/ws`);
    console.log(`📡 Device API: http://localhost:${PORT}/register`);
    console.log(`🔍 Network Discovery: ${config.discovery.enabled ? 'Enabled' : 'Disabled'}`);
    
    await this.app.listen({ port: PORT });
  }

  async stop(): Promise<void> {
    console.log("🛑 Shutting down server...");
    
    this.deviceManager.stop();
    this.commandQueue.stop();
    await this.networkDiscovery.stop();
    this.wsHandler.closeAllConnections();
    
    console.log("✅ Server stopped");
  }
}

// Handle graceful shutdown
const server = new WiFiDeviceServer();

Deno.addSignalListener("SIGINT", async () => {
  await server.stop();
  Deno.exit(0);
});

// Only add SIGTERM listener on non-Windows platforms
if (Deno.build.os !== "windows") {
  Deno.addSignalListener("SIGTERM", async () => {
    await server.stop();
    Deno.exit(0);
  });
}

// Start the server
if (import.meta.main) {
  try {
    await server.start();
  } catch (error) {
    console.error("❌ Failed to start server:", error);
    Deno.exit(1);
  }
}