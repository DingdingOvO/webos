/**
 * WebOS Taskbar Component
 * 
 * Renders the bottom taskbar with:
 * - Start button
 * - Running app buttons
 * - System tray (network, volume, battery, clock)
 */

export interface TaskbarApp {
  id: string;
  name: string;
  icon: string;
  active: boolean;
}

export class Taskbar {
  private height: number;
  private apps: TaskbarApp[] = [];

  constructor(height: number = 48) {
    this.height = height;
  }

  addApp(app: TaskbarApp): void {
    this.apps.push(app);
  }

  removeApp(id: string): void {
    this.apps = this.apps.filter(a => a.id !== id);
  }

  setActive(id: string): void {
    for (const app of this.apps) {
      app.active = app.id === id;
    }
  }

  get taskbarHeight(): number { return this.height; }
  get runningApps(): TaskbarApp[] { return this.apps; }
}
