/**
 * WebOS Start Menu Component
 * 
 * The start menu provides:
 * - Search functionality
 * - Pinned apps grid
 * - All apps list
 * - Power options (Sleep, Restart, Shutdown)
 * - Settings shortcut
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
    this.apps = [
      { id: 'calculator', name: 'Calculator', icon: '🔢', category: 'Utilities', pinned: true },
      { id: 'paint', name: 'Paint', icon: '🎨', category: 'Graphics', pinned: true },
      { id: 'browser', name: 'Browser', icon: '🌐', category: 'Internet', pinned: true },
      { id: 'appstore', name: 'App Store', icon: '🏪', category: 'System', pinned: true },
      { id: 'terminal', name: 'Terminal', icon: '💻', category: 'System', pinned: true },
      { id: 'settings', name: 'Settings', icon: '⚙️', category: 'System', pinned: true },
    ];
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

  setSearch(query: string): void {
    this.searchQuery = query;
  }
}
