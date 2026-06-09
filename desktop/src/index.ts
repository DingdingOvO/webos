/**
 * WebOS Desktop Entry Point
 * 
 * Initializes and starts the desktop environment.
 * This module bridges the WASM kernel world with the visual desktop.
 */

import { Desktop } from './desktop';

export async function startDesktop(canvas: HTMLCanvasElement): Promise<Desktop> {
  const desktop = new Desktop(canvas);
  await desktop.init();
  desktop.start();
  console.log('[WebOS Desktop] Started');
  return desktop;
}

export { Desktop } from './desktop';
