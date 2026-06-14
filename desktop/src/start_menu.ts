/**
 * WebOS Start Menu Component
 * 
 * The start menu provides:
 * - Search functionality
 * - Pinned apps grid (all 10 pre-installed apps)
 * - All apps list with categories
 * - Power options (Sleep, Restart, Shutdown)
 * - Settings shortcut
 *
 * Version: 0.0.1beta
 */

export interface StartMenuApp {
  id: string;
  name: string;
  icon: string;
  category: string;
  pinned: boolean;
}

export class StartMenu {
  private _isOpen: boolean = false;
  private apps: StartMenuApp[] = [];
  private searchQuery: string = '';

  constructor() {
    // All 10 pre-installed apps
    this.apps = [
      { id: 'terminal',      name: 'Terminal',        icon: '💻', category: 'System',     pinned: true },
      { id: 'filemanager',   name: 'File Manager',    icon: '📁', category: 'Utilities',  pinned: true },
      { id: 'texteditor',    name: 'Text Editor',     icon: '📝', category: 'Utilities',  pinned: true },
      { id: 'calculator',    name: 'Calculator',      icon: '🔢', category: 'Utilities',  pinned: true },
      { id: 'paint',         name: 'Paint',           icon: '🎨', category: 'Graphics',   pinned: true },
      { id: 'musicplayer',   name: 'Music Player',    icon: '🎵', category: 'Multimedia', pinned: true },
      { id: 'settings',      name: 'Settings',        icon: '⚙️', category: 'System',     pinned: true },
      { id: 'sysmonitor',    name: 'System Monitor',  icon: '📊', category: 'System',     pinned: true },
      { id: 'browser',       name: 'Browser',         icon: '🌐', category: 'Internet',   pinned: true },
      { id: 'appstore',      name: 'App Store',       icon: '🏪', category: 'System',     pinned: true },
    ];
  }

  addApp(app: StartMenuApp): void {
    // Check if already exists
    if (!this.apps.find(a => a.id === app.id)) {
      this.apps.push(app);
    }
  }

  removeApp(id: string): void {
    this.apps = this.apps.filter(a => a.id !== id);
  }

  toggle(): void { this._isOpen = !this._isOpen; }
  open(): void { this._isOpen = true; }
  close(): void { this._isOpen = false; }

  get isOpen(): boolean { return this._isOpen; }
  
  get pinnedApps(): StartMenuApp[] {
    return this.apps.filter(a => a.pinned);
  }

  get allApps(): StartMenuApp[] {
    if (this.searchQuery) {
      const q = this.searchQuery.toLowerCase();
      return this.apps.filter(a => a.name.toLowerCase().includes(q));
    }
    return this.apps;
  }

  get appsByCategory(): Record<string, StartMenuApp[]> {
    const result: Record<string, StartMenuApp[]> = {};
    const filtered = this.allApps;
    for (const app of filtered) {
      if (!result[app.category]) {
        result[app.category] = [];
      }
      result[app.category].push(app);
    }
    return result;
  }

  setSearch(query: string): void {
    this.searchQuery = query;
  }
}
