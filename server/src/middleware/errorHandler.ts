import { Context, Next } from "oak";
import { ApiResponse } from "../types/api.ts";

export async function errorHandler(ctx: Context, next: Next) {
  try {
    await next();
  } catch (error) {
    console.error("Request error:", error);
    
    const response: ApiResponse = {
      success: false,
      error: error instanceof Error ? error.message : "Internal server error",
      timestamp: new Date()
    };
    
    ctx.response.status = 500;
    ctx.response.body = response;
  }
}

export function createApiResponse<T>(data: T): ApiResponse<T> {
  return {
    success: true,
    data,
    timestamp: new Date()
  };
}

export function createErrorResponse(error: string, status = 400): { status: number; response: ApiResponse } {
  return {
    status,
    response: {
      success: false,
      error,
      timestamp: new Date()
    }
  };
}