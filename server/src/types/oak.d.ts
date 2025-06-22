// Type declarations for Oak framework extensions

import "https://deno.land/x/oak@v12.6.1/mod.ts";

declare module "https://deno.land/x/oak@v12.6.1/mod.ts" {
  interface Context {
    render: (template: string, data?: any) => Promise<void>;
  }
}