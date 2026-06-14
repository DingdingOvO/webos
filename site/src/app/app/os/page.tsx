'use client';

import { useState, useEffect, useCallback, useRef } from 'react';

// ============ FILE SYSTEM ============
interface FSNode {
  name: string;
  type: 'file' | 'directory';
  content?: string;
  children?: { [key: string]: FSNode };
  createdAt: number;
  modifiedAt: number;
}

class FileSystem {
  private root: FSNode;

  constructor() {
    let loaded = false;
    if (typeof window !== 'undefined') {
      try {
        const saved = localStorage.getItem('webos-fs');
        if (saved) {
          this.root = JSON.parse(saved);
          loaded = true;
        }
      } catch {
        // ignore parse errors
      }
    }
    if (!loaded) {
      this.root = this.createDefault();
      this.save();
    }
  }

  private createDefault(): FSNode {
    const now = Date.now();
    const dir = (name: string, children: { [key: string]: FSNode }): FSNode => ({
      name, type: 'directory', children, createdAt: now, modifiedAt: now,
    });
    const file = (name: string, content: string): FSNode => ({
      name, type: 'file', content, createdAt: now, modifiedAt: now,
    });

    return dir('/', {
      home: dir('home', {
        user: dir('user', {
          Desktop: dir('Desktop', {}),
          Documents: dir('Documents', {
            'notes.txt': file('notes.txt', 'Welcome to WebOS Documents!\nThis is your personal document folder.'),
          }),
          Downloads: dir('Downloads', {}),
          'readme.txt': file('readme.txt', '欢迎使用 WebOS!\n\n这是一个基于浏览器的操作系统模拟器。\n\n版本: 0.0.1beta\n\n在终端中输入 help 查看可用命令。'),
          '.bashrc': file('.bashrc', '# WebOS Bash Config\nexport PS1="user@webos:~$ "\nexport PATH="/usr/bin:/usr/local/bin"\nalias ll="ls -la"\nalias cls="clear"'),
          '.profile': file('.profile', '# WebOS User Profile\necho "Welcome back, user!"'),
          'hello.sh': file('hello.sh', '#!/bin/bash\necho "Hello from WebOS!"'),
        }),
      }),
      etc: dir('etc', {
        hostname: file('hostname', 'webos'),
        version: file('version', '0.0.1beta'),
        'os-release': file('os-release', 'NAME="WebOS"\nVERSION="0.0.1beta"\nID=webos\nPRETTY_NAME="WebOS 0.0.1beta"'),
        passwd: file('passwd', 'root:x:0:0:root:/root:/bin/bash\nuser:x:1000:1000:User:/home/user:/bin/bash'),
      }),
      tmp: dir('tmp', {}),
      usr: dir('usr', {
        bin: dir('bin', {}),
        lib: dir('lib', {}),
        share: dir('share', {}),
        local: dir('local', {
          bin: dir('bin', {}),
        }),
      }),
      var: dir('var', {
        log: dir('log', {
          'syslog': file('syslog', '[INFO] WebOS started at ' + new Date().toISOString()),
          'dmesg': file('dmesg', 'WebOS kernel loaded.\nFilesystem mounted.\nNetwork interface initialized.'),
        }),
        tmp: dir('tmp', {}),
      }),
      sys: dir('sys', {
        kernel: dir('kernel', {
          version: file('version', 'WebOS 0.0.1beta'),
        }),
        devices: dir('devices', {}),
      }),
      dev: dir('dev', {}),
      proc: dir('proc', {
        cpuinfo: file('cpuinfo', 'processor: WebCore Virtual CPU\ncores: 4\nMHz: 3000'),
        meminfo: file('meminfo', 'MemTotal: 8192 MB\nMemFree: 4096 MB'),
      }),
      root: dir('root', {}),
    });
  }

  private save() {
    if (typeof window !== 'undefined') {
      try {
        localStorage.setItem('webos-fs', JSON.stringify(this.root));
      } catch {
        // ignore quota errors
      }
    }
  }

  normalizePath(path: string): string {
    if (!path.startsWith('/')) path = '/' + path;
    const parts = path.split('/').filter(p => p !== '' && p !== '.');
    const resolved: string[] = [];
    for (const part of parts) {
      if (part === '..') {
        resolved.pop();
      } else {
        resolved.push(part);
      }
    }
    return '/' + resolved.join('/');
  }

  resolve(path: string): FSNode | null {
    const normalized = this.normalizePath(path);
    if (normalized === '/') return this.root;
    const parts = normalized.split('/').filter(p => p !== '');
    let current: FSNode = this.root;
    for (const part of parts) {
      if (current.type !== 'directory' || !current.children || !current.children[part]) {
        return null;
      }
      current = current.children[part];
    }
    return current;
  }

  ls(path: string): string[] {
    const node = this.resolve(path);
    if (!node || node.type !== 'directory' || !node.children) return [];
    return Object.keys(node.children);
  }

  read(path: string): string | null {
    const node = this.resolve(path);
    if (!node || node.type !== 'file') return null;
    return node.content ?? '';
  }

  write(path: string, content: string): boolean {
    const normalized = this.normalizePath(path);
    const parts = normalized.split('/').filter(p => p !== '');
    if (parts.length === 0) return false;
    const fileName = parts.pop()!;
    const dirPath = '/' + parts.join('/');
    const dirNode = this.resolve(dirPath);
    if (!dirNode || dirNode.type !== 'directory' || !dirNode.children) return false;
    if (dirNode.children[fileName] && dirNode.children[fileName].type === 'directory') return false;
    if (dirNode.children[fileName]) {
      dirNode.children[fileName].content = content;
      dirNode.children[fileName].modifiedAt = Date.now();
    } else {
      dirNode.children[fileName] = {
        name: fileName,
        type: 'file',
        content,
        createdAt: Date.now(),
        modifiedAt: Date.now(),
      };
    }
    this.save();
    return true;
  }

  mkdir(path: string): boolean {
    const normalized = this.normalizePath(path);
    const parts = normalized.split('/').filter(p => p !== '');
    if (parts.length === 0) return false;
    const dirName = parts.pop()!;
    const parentPath = '/' + parts.join('/');
    const parent = this.resolve(parentPath);
    if (!parent || parent.type !== 'directory' || !parent.children) return false;
    if (parent.children[dirName]) return false;
    parent.children[dirName] = {
      name: dirName,
      type: 'directory',
      children: {},
      createdAt: Date.now(),
      modifiedAt: Date.now(),
    };
    this.save();
    return true;
  }

  rm(path: string): boolean {
    const normalized = this.normalizePath(path);
    const parts = normalized.split('/').filter(p => p !== '');
    if (parts.length === 0) return false;
    const name = parts.pop()!;
    const parentPath = '/' + parts.join('/');
    const parent = this.resolve(parentPath);
    if (!parent || parent.type !== 'directory' || !parent.children) return false;
    if (!parent.children[name]) return false;
    const target = parent.children[name];
    if (target.type === 'directory' && target.children && Object.keys(target.children).length > 0) return false;
    delete parent.children[name];
    this.save();
    return true;
  }

  exists(path: string): boolean {
    return this.resolve(path) !== null;
  }

  isDir(path: string): boolean {
    const node = this.resolve(path);
    return node !== null && node.type === 'directory';
  }

  stat(path: string): { name: string; type: string; size: number; modified: string } | null {
    const node = this.resolve(path);
    if (!node) return null;
    return {
      name: node.name,
      type: node.type,
      size: node.type === 'file' ? (node.content?.length ?? 0) : Object.keys(node.children ?? {}).length,
      modified: new Date(node.modifiedAt).toLocaleString('zh-CN'),
    };
  }
}

// ============ WINDOW MANAGER ============
interface WindowState {
  id: string;
  appId: string;
  title: string;
  x: number;
  y: number;
  width: number;
  height: number;
  minWidth: number;
  minHeight: number;
  isMinimized: boolean;
  isMaximized: boolean;
  zIndex: number;
  prevBounds?: { x: number; y: number; width: number; height: number };
}

// ============ APP COMPONENTS ============

// ---------- Terminal App ----------
function TerminalApp({ fs, initialCwd, onCwdChange }: { fs: FileSystem; initialCwd: string; onCwdChange: (cwd: string) => void }) {
  const [lines, setLines] = useState<string[]>([
    'WebOS Terminal v0.0.1beta',
    '输入 help 查看可用命令',
    '',
  ]);
  const [input, setInput] = useState('');
  const [cwd, setCwd] = useState(initialCwd);
  const [history, setHistory] = useState<string[]>([]);
  const [historyIndex, setHistoryIndex] = useState(-1);
  const inputRef = useRef<HTMLInputElement>(null);
  const scrollRef = useRef<HTMLDivElement>(null);

  useEffect(() => {
    if (scrollRef.current) {
      scrollRef.current.scrollTop = scrollRef.current.scrollHeight;
    }
  }, [lines]);

  useEffect(() => {
    inputRef.current?.focus();
  }, []);

  const resolvePath = (path: string): string => {
    if (path.startsWith('/')) return fs.normalizePath(path);
    if (path === '~' || path.startsWith('~/')) {
      const rest = path === '~' ? '' : path.slice(2);
      return fs.normalizePath('/home/user/' + rest);
    }
    return fs.normalizePath(cwd + '/' + path);
  };

  const getPrompt = () => {
    let displayCwd = cwd;
    if (cwd.startsWith('/home/user')) {
      displayCwd = '~' + cwd.slice('/home/user'.length);
    }
    return `user@webos:${displayCwd}$ `;
  };

  const executeCommand = useCallback((cmd: string) => {
    const trimmed = cmd.trim();
    if (!trimmed) {
      setLines(prev => [...prev, getPrompt()]);
      return;
    }

    const newLines = [...lines, getPrompt() + trimmed];
    setHistory(prev => [...prev, trimmed]);
    setHistoryIndex(-1);

    // Handle echo with redirect
    if (trimmed.startsWith('echo ') && trimmed.includes('>')) {
      const redirectIndex = trimmed.lastIndexOf('>');
      const text = trimmed.slice(5, redirectIndex).trim().replace(/^["']|["']$/g, '');
      const filePath = trimmed.slice(redirectIndex + 1).trim();
      if (filePath) {
        const fullPath = resolvePath(filePath);
        if (fs.write(fullPath, text)) {
          newLines.push('');
        } else {
          newLines.push(`写入失败: ${filePath}`);
        }
      }
      setLines(newLines);
      return;
    }

    const parts = trimmed.split(/\s+/);
    const command = parts[0];
    const args = parts.slice(1);

    switch (command) {
      case 'ls': {
        const targetPath = args[0] ? resolvePath(args[0]) : cwd;
        const items = fs.ls(targetPath);
        if (items.length === 0) {
          const node = fs.resolve(targetPath);
          if (!node) {
            newLines.push(`ls: 无法访问 '${args[0] || targetPath}': 没有那个文件或目录`);
          }
        } else {
          const formatted = items.map(name => {
            const fullPath = fs.normalizePath(targetPath + '/' + name);
            const isDir = fs.isDir(fullPath);
            return isDir ? `\x1b[34m${name}/\x1b[0m` : name;
          });
          newLines.push(formatted.join('  '));
        }
        break;
      }
      case 'cd': {
        if (!args[0] || args[0] === '~') {
          setCwd('/home/user');
          onCwdChange('/home/user');
          break;
        }
        const targetPath = resolvePath(args[0]);
        if (fs.isDir(targetPath)) {
          setCwd(targetPath);
          onCwdChange(targetPath);
        } else {
          newLines.push(`cd: ${args[0]}: 没有那个文件或目录`);
        }
        break;
      }
      case 'pwd': {
        newLines.push(cwd);
        break;
      }
      case 'mkdir': {
        if (!args[0]) {
          newLines.push('mkdir: 缺少操作数');
        } else {
          const fullPath = resolvePath(args[0]);
          if (!fs.mkdir(fullPath)) {
            newLines.push(`mkdir: 无法创建目录 '${args[0]}'`);
          }
        }
        break;
      }
      case 'rmdir': {
        if (!args[0]) {
          newLines.push('rmdir: 缺少操作数');
        } else {
          const fullPath = resolvePath(args[0]);
          if (!fs.rm(fullPath)) {
            newLines.push(`rmdir: 无法删除 '${args[0]}' (目录非空或不存在)`);
          }
        }
        break;
      }
      case 'rm': {
        if (!args[0]) {
          newLines.push('rm: 缺少操作数');
        } else {
          const fullPath = resolvePath(args[0]);
          if (!fs.rm(fullPath)) {
            newLines.push(`rm: 无法删除 '${args[0]}'`);
          }
        }
        break;
      }
      case 'cat': {
        if (!args[0]) {
          newLines.push('cat: 缺少操作数');
        } else {
          const fullPath = resolvePath(args[0]);
          const content = fs.read(fullPath);
          if (content === null) {
            newLines.push(`cat: ${args[0]}: 没有那个文件或目录`);
          } else {
            newLines.push(content);
          }
        }
        break;
      }
      case 'echo': {
        newLines.push(args.join(' ').replace(/^["']|["']$/g, ''));
        break;
      }
      case 'touch': {
        if (!args[0]) {
          newLines.push('touch: 缺少操作数');
        } else {
          const fullPath = resolvePath(args[0]);
          if (!fs.exists(fullPath)) {
            fs.write(fullPath, '');
          }
        }
        break;
      }
      case 'clear': {
        setLines([]);
        return;
      }
      case 'help': {
        newLines.push('可用命令:');
        newLines.push('  ls [路径]        列出目录内容');
        newLines.push('  cd <路径>        切换目录');
        newLines.push('  pwd              显示当前目录');
        newLines.push('  mkdir <名称>     创建目录');
        newLines.push('  rmdir <名称>     删除空目录');
        newLines.push('  rm <名称>        删除文件或空目录');
        newLines.push('  cat <文件>       显示文件内容');
        newLines.push('  echo <文本>      输出文本');
        newLines.push('  echo <文本> > <文件>  写入文件');
        newLines.push('  touch <名称>     创建空文件');
        newLines.push('  clear            清屏');
        newLines.push('  help             显示帮助');
        newLines.push('  whoami           显示当前用户');
        newLines.push('  date             显示日期时间');
        newLines.push('  uname            显示系统信息');
        break;
      }
      case 'whoami': {
        newLines.push('user');
        break;
      }
      case 'date': {
        newLines.push(new Date().toLocaleString('zh-CN', {
          weekday: 'long', year: 'numeric', month: 'long', day: 'numeric',
          hour: '2-digit', minute: '2-digit', second: '2-digit',
        }));
        break;
      }
      case 'uname': {
        if (args[0] === '-a') {
          newLines.push('WebOS 0.0.1beta webos x86_64 WebCore/Virtual');
        } else {
          newLines.push('WebOS');
        }
        break;
      }
      default: {
        newLines.push(`${command}: 命令未找到`);
        break;
      }
    }

    newLines.push('');
    setLines(newLines);
  // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [cwd, fs, lines, onCwdChange]);

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === 'Enter') {
      e.preventDefault();
      executeCommand(input);
      setInput('');
    } else if (e.key === 'ArrowUp') {
      e.preventDefault();
      if (history.length > 0) {
        const newIndex = historyIndex === -1 ? history.length - 1 : Math.max(0, historyIndex - 1);
        setHistoryIndex(newIndex);
        setInput(history[newIndex]);
      }
    } else if (e.key === 'ArrowDown') {
      e.preventDefault();
      if (historyIndex !== -1) {
        const newIndex = historyIndex + 1;
        if (newIndex >= history.length) {
          setHistoryIndex(-1);
          setInput('');
        } else {
          setHistoryIndex(newIndex);
          setInput(history[newIndex]);
        }
      }
    }
  };

  const renderLine = (line: string) => {
    // Simple ANSI-like color support for directory highlighting
    const parts: React.ReactNode[] = [];
    let remaining = line;
    let key = 0;
    while (remaining.length > 0) {
      const colorStart = remaining.indexOf('\x1b[34m');
      const colorEnd = remaining.indexOf('\x1b[0m');
      if (colorStart !== -1 && colorEnd !== -1 && colorEnd > colorStart) {
        if (colorStart > 0) {
          parts.push(<span key={key++}>{remaining.slice(0, colorStart)}</span>);
        }
        parts.push(<span key={key++} style={{ color: '#5b9bd5' }}>{remaining.slice(colorStart + 5, colorEnd)}</span>);
        remaining = remaining.slice(colorEnd + 4);
      } else {
        parts.push(<span key={key++}>{remaining}</span>);
        break;
      }
    }
    return parts;
  };

  return (
    <div
      style={{ display: 'flex', flexDirection: 'column', height: '100%', background: '#1e1e1e', color: '#cccccc', fontFamily: "'SF Mono', 'Fira Code', 'Cascadia Code', Consolas, monospace", fontSize: '13px' }}
      onClick={() => inputRef.current?.focus()}
    >
      <div ref={scrollRef} style={{ flex: 1, overflowY: 'auto', padding: '8px 12px', whiteSpace: 'pre-wrap', wordBreak: 'break-all' }}>
        {lines.map((line, i) => (
          <div key={i}>{renderLine(line)}</div>
        ))}
        <div style={{ display: 'flex', alignItems: 'center' }}>
          <span style={{ color: '#6a9955', whiteSpace: 'pre' }}>{getPrompt()}</span>
          <input
            ref={inputRef}
            value={input}
            onChange={e => setInput(e.target.value)}
            onKeyDown={handleKeyDown}
            style={{
              flex: 1, background: 'transparent', border: 'none', outline: 'none',
              color: '#cccccc', fontFamily: 'inherit', fontSize: 'inherit', caretColor: '#cccccc',
            }}
            spellCheck={false}
            autoComplete="off"
            autoCapitalize="off"
          />
        </div>
      </div>
    </div>
  );
}

// ---------- File Manager App ----------
function FileManagerApp({ fs }: { fs: FileSystem }) {
  const [currentPath, setCurrentPath] = useState('/home/user');
  const [contextMenu, setContextMenu] = useState<{ x: number; y: number } | null>(null);
  const [newFolderDialog, setNewFolderDialog] = useState(false);
  const [newFolderName, setNewFolderName] = useState('');

  const items = fs.ls(currentPath);
  const breadcrumbParts = currentPath === '/' ? ['/'] : currentPath.split('/').filter(p => p !== '');

  const navigateTo = (path: string) => {
    if (fs.isDir(path)) {
      setCurrentPath(path);
      setContextMenu(null);
    }
  };

  const handleItemDoubleClick = (name: string) => {
    const fullPath = fs.normalizePath(currentPath + '/' + name);
    if (fs.isDir(fullPath)) {
      navigateTo(fullPath);
    }
  };

  const handleContextMenu = (e: React.MouseEvent) => {
    e.preventDefault();
    setContextMenu({ x: e.clientX, y: e.clientY });
  };

  const createNewFolder = () => {
    if (newFolderName.trim()) {
      const fullPath = fs.normalizePath(currentPath + '/' + newFolderName.trim());
      fs.mkdir(fullPath);
      setNewFolderDialog(false);
      setNewFolderName('');
      // Force re-render by updating path
      setCurrentPath(currentPath);
    }
  };

  const [deleteTarget, setDeleteTarget] = useState<string | null>(null);

  const deleteItem = (name: string) => {
    const fullPath = fs.normalizePath(currentPath + '/' + name);
    fs.rm(fullPath);
    setDeleteTarget(null);
    setCurrentPath(currentPath + ''); // force re-render
  };

  const sortedItems = [...items].sort((a, b) => {
    const aIsDir = fs.isDir(fs.normalizePath(currentPath + '/' + a));
    const bIsDir = fs.isDir(fs.normalizePath(currentPath + '/' + b));
    if (aIsDir && !bIsDir) return -1;
    if (!aIsDir && bIsDir) return 1;
    return a.localeCompare(b);
  });

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', background: '#1e1e1e', color: '#cccccc' }} onClick={() => setContextMenu(null)}>
      {/* Breadcrumb */}
      <div style={{ display: 'flex', alignItems: 'center', padding: '8px 12px', borderBottom: '1px solid #333', background: '#252526', gap: '4px', flexWrap: 'wrap' }}>
        {breadcrumbParts.map((part, i) => {
          const pathUpTo = part === '/' ? '/' : '/' + breadcrumbParts.slice(0, i + 1).join('/');
          return (
            <span key={i} style={{ display: 'flex', alignItems: 'center', gap: '4px' }}>
              {i > 0 && <span style={{ color: '#666' }}>/</span>}
              <button
                onClick={() => navigateTo(pathUpTo)}
                style={{ background: 'none', border: 'none', color: '#5b9bd5', cursor: 'pointer', fontSize: '13px', padding: '2px 4px', borderRadius: '3px' }}
                onMouseEnter={e => (e.currentTarget.style.background = '#333')}
                onMouseLeave={e => (e.currentTarget.style.background = 'none')}
              >
                {part === '/' ? '/' : part}
              </button>
            </span>
          );
        })}
      </div>

      {/* Content */}
      <div style={{ flex: 1, overflowY: 'auto', padding: '12px' }} onContextMenu={handleContextMenu}>
        {/* Up directory */}
        {currentPath !== '/' && (
          <div
            style={{ display: 'inline-flex', flexDirection: 'column', alignItems: 'center', width: '90px', padding: '8px 4px', cursor: 'pointer', borderRadius: '6px', margin: '4px', verticalAlign: 'top' }}
            onMouseEnter={e => (e.currentTarget.style.background = '#333')}
            onMouseLeave={e => (e.currentTarget.style.background = 'transparent')}
            onDoubleClick={() => {
              const parts = currentPath.split('/').filter(p => p !== '');
              parts.pop();
              navigateTo('/' + parts.join('/'));
            }}
          >
            <span style={{ fontSize: '32px' }}>📁</span>
            <span style={{ fontSize: '11px', marginTop: '4px', color: '#aaa', wordBreak: 'break-all', textAlign: 'center' }}>..</span>
          </div>
        )}

        {sortedItems.map(name => {
          const fullPath = fs.normalizePath(currentPath + '/' + name);
          const isDir = fs.isDir(fullPath);
          return (
            <div
              key={name}
              style={{ display: 'inline-flex', flexDirection: 'column', alignItems: 'center', width: '90px', padding: '8px 4px', cursor: 'pointer', borderRadius: '6px', margin: '4px', verticalAlign: 'top' }}
              onMouseEnter={e => (e.currentTarget.style.background = '#333')}
              onMouseLeave={e => (e.currentTarget.style.background = 'transparent')}
              onDoubleClick={() => handleItemDoubleClick(name)}
              onContextMenu={e => {
                e.preventDefault();
                e.stopPropagation();
                setContextMenu({ x: e.clientX, y: e.clientY });
              }}
            >
              <span style={{ fontSize: '32px' }}>{isDir ? '📁' : '📄'}</span>
              <span style={{ fontSize: '11px', marginTop: '4px', color: isDir ? '#cccccc' : '#aaa', wordBreak: 'break-all', textAlign: 'center' }}>{name}</span>
            </div>
          );
        })}

        {items.length === 0 && currentPath === '/' && (
          <div style={{ color: '#666', textAlign: 'center', padding: '40px' }}>空目录</div>
        )}
      </div>

      {/* Status bar */}
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between', padding: '4px 12px', borderTop: '1px solid #333', background: '#252526', fontSize: '12px', color: '#888' }}>
        <span>{items.length} 项</span>
        <div style={{ display: 'flex', gap: '8px' }}>
          <button
            onClick={() => setNewFolderDialog(true)}
            style={{ background: 'none', border: '1px solid #444', color: '#ccc', padding: '2px 8px', borderRadius: '3px', cursor: 'pointer', fontSize: '11px' }}
          >
            新建文件夹
          </button>
          <button
            onClick={() => setCurrentPath(currentPath)}
            style={{ background: 'none', border: '1px solid #444', color: '#ccc', padding: '2px 8px', borderRadius: '3px', cursor: 'pointer', fontSize: '11px' }}
          >
            刷新
          </button>
        </div>
      </div>

      {/* Context menu */}
      {contextMenu && (
        <div style={{ position: 'fixed', left: contextMenu.x, top: contextMenu.y, background: '#2d2d30', border: '1px solid #454545', borderRadius: '6px', padding: '4px 0', zIndex: 10000, minWidth: '160px', boxShadow: '0 8px 24px rgba(0,0,0,0.4)' }}>
          <button
            onClick={() => { setNewFolderDialog(true); setContextMenu(null); }}
            style={{ display: 'block', width: '100%', padding: '6px 16px', background: 'none', border: 'none', color: '#ccc', cursor: 'pointer', textAlign: 'left', fontSize: '13px' }}
            onMouseEnter={e => (e.currentTarget.style.background = '#094771')}
            onMouseLeave={e => (e.currentTarget.style.background = 'none')}
          >
            📁 新建文件夹
          </button>
          {deleteTarget && (
            <button
              onClick={() => { deleteItem(deleteTarget); setContextMenu(null); }}
              style={{ display: 'block', width: '100%', padding: '6px 16px', background: 'none', border: 'none', color: '#f48771', cursor: 'pointer', textAlign: 'left', fontSize: '13px' }}
              onMouseEnter={e => (e.currentTarget.style.background = '#094771')}
              onMouseLeave={e => (e.currentTarget.style.background = 'none')}
            >
              🗑️ 删除 "{deleteTarget}"
            </button>
          )}
        </div>
      )}

      {/* New folder dialog */}
      {newFolderDialog && (
        <div style={{ position: 'absolute', inset: 0, background: 'rgba(0,0,0,0.5)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 10001 }}>
          <div style={{ background: '#2d2d30', border: '1px solid #454545', borderRadius: '8px', padding: '20px', minWidth: '280px' }}>
            <div style={{ marginBottom: '12px', fontSize: '14px', fontWeight: 600 }}>新建文件夹</div>
            <input
              value={newFolderName}
              onChange={e => setNewFolderName(e.target.value)}
              onKeyDown={e => { if (e.key === 'Enter') createNewFolder(); if (e.key === 'Escape') setNewFolderDialog(false); }}
              placeholder="文件夹名称"
              autoFocus
              style={{ width: '100%', padding: '6px 10px', background: '#1e1e1e', border: '1px solid #454545', borderRadius: '4px', color: '#ccc', outline: 'none', fontSize: '13px', marginBottom: '12px' }}
            />
            <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '8px' }}>
              <button onClick={() => setNewFolderDialog(false)} style={{ padding: '4px 16px', background: 'none', border: '1px solid #454545', color: '#ccc', borderRadius: '4px', cursor: 'pointer', fontSize: '13px' }}>取消</button>
              <button onClick={createNewFolder} style={{ padding: '4px 16px', background: '#0e639c', border: 'none', color: '#fff', borderRadius: '4px', cursor: 'pointer', fontSize: '13px' }}>创建</button>
            </div>
          </div>
        </div>
      )}

      {/* Delete confirmation dialog */}
      {deleteTarget && !contextMenu && (
        <div style={{ position: 'absolute', inset: 0, background: 'rgba(0,0,0,0.5)', display: 'flex', alignItems: 'center', justifyContent: 'center', zIndex: 10001 }} onClick={() => setDeleteTarget(null)}>
          <div style={{ background: '#2d2d30', border: '1px solid #454545', borderRadius: '8px', padding: '20px', minWidth: '280px' }} onClick={e => e.stopPropagation()}>
            <div style={{ marginBottom: '12px', fontSize: '14px', fontWeight: 600 }}>确认删除</div>
            <div style={{ marginBottom: '12px', fontSize: '13px', color: '#aaa' }}>确定要删除 "{deleteTarget}" 吗？</div>
            <div style={{ display: 'flex', justifyContent: 'flex-end', gap: '8px' }}>
              <button onClick={() => setDeleteTarget(null)} style={{ padding: '4px 16px', background: 'none', border: '1px solid #454545', color: '#ccc', borderRadius: '4px', cursor: 'pointer', fontSize: '13px' }}>取消</button>
              <button onClick={() => deleteItem(deleteTarget)} style={{ padding: '4px 16px', background: '#d32f2f', border: 'none', color: '#fff', borderRadius: '4px', cursor: 'pointer', fontSize: '13px' }}>删除</button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}

// ---------- Settings App ----------
const WALLPAPERS = [
  { name: 'Cosmic', value: 'linear-gradient(135deg, #0c0015, #1a0033, #0a1628)' },
  { name: 'Ocean', value: 'linear-gradient(135deg, #0a1628, #0d2137, #1a3a4a)' },
  { name: 'Sunset', value: 'linear-gradient(135deg, #1a0a00, #3a1a0a, #2a1500)' },
  { name: 'Forest', value: 'linear-gradient(135deg, #0a1a0a, #0d2a0d, #1a3a1a)' },
  { name: 'Aurora', value: 'linear-gradient(135deg, #0a0a2a, #0d1a3a, #1a2a4a)' },
  { name: 'Minimal', value: 'linear-gradient(135deg, #1a1a2e, #16213e, #0f3460)' },
];

function SettingsApp({ wallpaper, onWallpaperChange }: { wallpaper: string; onWallpaperChange: (wp: string) => void }) {
  const [activeTab, setActiveTab] = useState<'wallpaper' | 'about'>('wallpaper');

  return (
    <div style={{ display: 'flex', flexDirection: 'column', height: '100%', background: '#1e1e1e', color: '#cccccc' }}>
      {/* Tabs */}
      <div style={{ display: 'flex', borderBottom: '1px solid #333', background: '#252526' }}>
        <button
          onClick={() => setActiveTab('wallpaper')}
          style={{
            padding: '10px 20px', background: activeTab === 'wallpaper' ? '#1e1e1e' : 'transparent',
            border: 'none', borderBottom: activeTab === 'wallpaper' ? '2px solid #5b9bd5' : '2px solid transparent',
            color: activeTab === 'wallpaper' ? '#fff' : '#888', cursor: 'pointer', fontSize: '13px', fontWeight: 500,
          }}
        >
          壁纸
        </button>
        <button
          onClick={() => setActiveTab('about')}
          style={{
            padding: '10px 20px', background: activeTab === 'about' ? '#1e1e1e' : 'transparent',
            border: 'none', borderBottom: activeTab === 'about' ? '2px solid #5b9bd5' : '2px solid transparent',
            color: activeTab === 'about' ? '#fff' : '#888', cursor: 'pointer', fontSize: '13px', fontWeight: 500,
          }}
        >
          关于
        </button>
      </div>

      {/* Content */}
      <div style={{ flex: 1, overflowY: 'auto', padding: '20px' }}>
        {activeTab === 'wallpaper' && (
          <div>
            <h3 style={{ fontSize: '15px', fontWeight: 600, marginBottom: '16px', color: '#fff' }}>选择壁纸</h3>
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(3, 1fr)', gap: '12px' }}>
              {WALLPAPERS.map(wp => (
                <div
                  key={wp.name}
                  onClick={() => onWallpaperChange(wp.value)}
                  style={{
                    height: '80px', borderRadius: '8px', background: wp.value, cursor: 'pointer',
                    border: wallpaper === wp.value ? '3px solid #5b9bd5' : '3px solid transparent',
                    display: 'flex', alignItems: 'flex-end', justifyContent: 'center', paddingBottom: '8px',
                    transition: 'border-color 0.2s',
                  }}
                >
                  <span style={{ fontSize: '12px', color: '#aaa', textShadow: '0 1px 3px rgba(0,0,0,0.8)' }}>{wp.name}</span>
                </div>
              ))}
            </div>
          </div>
        )}

        {activeTab === 'about' && (
          <div>
            <div style={{ textAlign: 'center', marginBottom: '24px' }}>
              <div style={{ fontSize: '48px', marginBottom: '8px' }}>🖥️</div>
              <h2 style={{ fontSize: '22px', fontWeight: 700, color: '#fff' }}>WebOS</h2>
              <p style={{ color: '#888', fontSize: '13px' }}>版本 0.0.1beta</p>
            </div>
            <div style={{ background: '#252526', borderRadius: '8px', padding: '16px' }}>
              <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: '13px' }}>
                <tbody>
                  {[
                    ['系统名称', 'WebOS'],
                    ['版本', '0.0.1beta'],
                    ['内核', 'WebCore Virtual'],
                    ['架构', 'x86_64 (WebAssembly)'],
                    ['内存', '8192 MB (虚拟)'],
                    ['处理器', 'WebCore Virtual CPU × 4'],
                    ['图形', 'Canvas 2D / WebGL'],
                    ['用户', 'user'],
                    ['主机名', 'webos'],
                  ].map(([key, value]) => (
                    <tr key={key}>
                      <td style={{ padding: '6px 12px 6px 0', color: '#888', whiteSpace: 'nowrap' }}>{key}</td>
                      <td style={{ padding: '6px 0', color: '#ccc' }}>{value}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}

// ============ BOOT SCREEN ============
function BootScreen({ onComplete }: { onComplete: () => void }) {
  const [progress, setProgress] = useState(0);
  const [message, setMessage] = useState('');

  useEffect(() => {
    const messages = [
      { text: '正在初始化内核...', delay: 0, progress: 20 },
      { text: '加载文件系统...', delay: 600, progress: 40 },
      { text: '启动窗口管理器...', delay: 1200, progress: 60 },
      { text: '加载桌面环境...', delay: 1800, progress: 80 },
      { text: '欢迎使用 WebOS', delay: 2400, progress: 100 },
    ];

    const timers: ReturnType<typeof setTimeout>[] = [];
    messages.forEach(({ text, delay, progress: prog }) => {
      timers.push(setTimeout(() => {
        setMessage(text);
        setProgress(prog);
      }, delay));
    });

    timers.push(setTimeout(() => {
      onComplete();
    }, 3000));

    return () => timers.forEach(clearTimeout);
  }, [onComplete]);

  return (
    <div style={{
      width: '100%', height: '100vh', display: 'flex', flexDirection: 'column',
      alignItems: 'center', justifyContent: 'center', background: '#09090b', color: '#fafafa',
      fontFamily: "'SF Mono', 'Fira Code', 'Cascadia Code', Consolas, monospace",
    }}>
      <div style={{ fontSize: '2.5rem', fontWeight: 800, marginBottom: '0.5rem', letterSpacing: '-0.02em' }}>WebOS</div>
      <div style={{ fontSize: '0.9rem', color: '#71717a', marginBottom: '2rem' }}>v0.0.1beta</div>
      <div style={{
        width: '36px', height: '36px', border: '3px solid #27272a', borderTopColor: '#5b9bd5',
        borderRadius: '50%', animation: 'boot-spin 0.8s linear infinite', marginBottom: '1.5rem',
      }} />
      <div style={{ fontSize: '0.85rem', color: '#71717a', marginBottom: '0.75rem', minHeight: '1.2em' }}>{message}</div>
      <div style={{ width: '200px', height: '3px', background: '#27272a', borderRadius: '2px', overflow: 'hidden' }}>
        <div style={{ width: `${progress}%`, height: '100%', background: '#5b9bd5', borderRadius: '2px', transition: 'width 0.3s ease' }} />
      </div>
      <style>{`@keyframes boot-spin { to { transform: rotate(360deg); } }`}</style>
    </div>
  );
}

// ============ WINDOW ============
function Window({
  windowState,
  isActive,
  onClose,
  onMinimize,
  onMaximize,
  onFocus,
  onMove,
  onResize,
  children,
}: {
  windowState: WindowState;
  isActive: boolean;
  onClose: () => void;
  onMinimize: () => void;
  onMaximize: () => void;
  onFocus: () => void;
  onMove: (x: number, y: number) => void;
  onResize: (w: number, h: number) => void;
  children: React.ReactNode;
}) {
  const dragRef = useRef<{ startX: number; startY: number; origX: number; origY: number } | null>(null);
  const resizeRef = useRef<{ startX: number; startY: number; origW: number; origH: number; origX: number; origY: number; dir: string } | null>(null);
  const windowRef = useRef<HTMLDivElement>(null);

  const handleTitleMouseDown = (e: React.MouseEvent) => {
    if (windowState.isMaximized) return;
    e.preventDefault();
    onFocus();
    dragRef.current = {
      startX: e.clientX,
      startY: e.clientY,
      origX: windowState.x,
      origY: windowState.y,
    };

    const handleMouseMove = (e: MouseEvent) => {
      if (!dragRef.current) return;
      const dx = e.clientX - dragRef.current.startX;
      const dy = e.clientY - dragRef.current.startY;
      onMove(dragRef.current.origX + dx, dragRef.current.origY + dy);
    };

    const handleMouseUp = () => {
      dragRef.current = null;
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };

    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
  };

  const handleResizeMouseDown = (e: React.MouseEvent, dir: string) => {
    if (windowState.isMaximized) return;
    e.preventDefault();
    e.stopPropagation();
    onFocus();
    resizeRef.current = {
      startX: e.clientX,
      startY: e.clientY,
      origW: windowState.width,
      origH: windowState.height,
      origX: windowState.x,
      origY: windowState.y,
      dir,
    };

    const handleMouseMove = (e: MouseEvent) => {
      if (!resizeRef.current) return;
      const dx = e.clientX - resizeRef.current.startX;
      const dy = e.clientY - resizeRef.current.startY;
      const { origW, origH, origX, origY, dir: d } = resizeRef.current;

      let newW = origW;
      let newH = origH;
      let newX = origX;
      let newY = origY;

      if (d.includes('e')) newW = Math.max(windowState.minWidth, origW + dx);
      if (d.includes('s')) newH = Math.max(windowState.minHeight, origH + dy);
      if (d.includes('w')) {
        newW = Math.max(windowState.minWidth, origW - dx);
        if (newW > windowState.minWidth) newX = origX + dx;
      }
      if (d.includes('n')) {
        newH = Math.max(windowState.minHeight, origH - dy);
        if (newH > windowState.minHeight) newY = origY + dy;
      }

      onMove(newX, newY);
      onResize(newW, newH);
    };

    const handleMouseUp = () => {
      resizeRef.current = null;
      document.removeEventListener('mousemove', handleMouseMove);
      document.removeEventListener('mouseup', handleMouseUp);
    };

    document.addEventListener('mousemove', handleMouseMove);
    document.addEventListener('mouseup', handleMouseUp);
  };

  if (windowState.isMinimized) return null;

  const style: React.CSSProperties = windowState.isMaximized
    ? { position: 'absolute', left: 0, top: 0, width: '100%', height: 'calc(100vh - 48px)', zIndex: windowState.zIndex, display: 'flex', flexDirection: 'column' }
    : { position: 'absolute', left: windowState.x, top: windowState.y, width: windowState.width, height: windowState.height, zIndex: windowState.zIndex, display: 'flex', flexDirection: 'column' };

  return (
    <div
      ref={windowRef}
      style={{
        ...style,
        borderRadius: windowState.isMaximized ? '0' : '8px',
        overflow: 'hidden',
        boxShadow: isActive ? '0 8px 32px rgba(0,0,0,0.5)' : '0 4px 16px rgba(0,0,0,0.3)',
        border: '1px solid #454545',
      }}
      onMouseDown={onFocus}
    >
      {/* Title bar */}
      <div
        style={{
          height: '32px', background: isActive ? '#323233' : '#2d2d30', display: 'flex',
          alignItems: 'center', padding: '0 8px', cursor: windowState.isMaximized ? 'default' : 'grab',
          userSelect: 'none', flexShrink: 0, borderBottom: '1px solid #454545',
        }}
        onMouseDown={handleTitleMouseDown}
        onDoubleClick={onMaximize}
      >
        <span style={{ fontSize: '12px', marginRight: '8px' }}>
          {windowState.appId === 'terminal' ? '⬛' : windowState.appId === 'filemanager' ? '📁' : '⚙️'}
        </span>
        <span style={{ flex: 1, fontSize: '12px', color: isActive ? '#fff' : '#888', fontWeight: 500, textAlign: 'center', overflow: 'hidden', textOverflow: 'ellipsis', whiteSpace: 'nowrap' }}>
          {windowState.title}
        </span>
        <div style={{ display: 'flex', gap: '0px' }}>
          <button
            onClick={e => { e.stopPropagation(); onMinimize(); }}
            style={{ width: '32px', height: '32px', background: 'none', border: 'none', color: '#aaa', cursor: 'pointer', fontSize: '14px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
            onMouseEnter={e => (e.currentTarget.style.background = '#444')}
            onMouseLeave={e => (e.currentTarget.style.background = 'none')}
          >—</button>
          <button
            onClick={e => { e.stopPropagation(); onMaximize(); }}
            style={{ width: '32px', height: '32px', background: 'none', border: 'none', color: '#aaa', cursor: 'pointer', fontSize: '12px', display: 'flex', alignItems: 'center', justifyContent: 'center' }}
            onMouseEnter={e => (e.currentTarget.style.background = '#444')}
            onMouseLeave={e => (e.currentTarget.style.background = 'none')}
          >□</button>
          <button
            onClick={e => { e.stopPropagation(); onClose(); }}
            style={{ width: '32px', height: '32px', background: 'none', border: 'none', color: '#aaa', cursor: 'pointer', fontSize: '16px', display: 'flex', alignItems: 'center', justifyContent: 'center', borderRadius: '0 8px 0 0' }}
            onMouseEnter={e => { e.currentTarget.style.background = '#e81123'; e.currentTarget.style.color = '#fff'; }}
            onMouseLeave={e => { e.currentTarget.style.background = 'none'; e.currentTarget.style.color = '#aaa'; }}
          >×</button>
        </div>
      </div>

      {/* Content */}
      <div style={{ flex: 1, overflow: 'hidden', position: 'relative' }}>
        {children}
      </div>

      {/* Resize handles */}
      {!windowState.isMaximized && (
        <>
          <div onMouseDown={e => handleResizeMouseDown(e, 'n')} style={{ position: 'absolute', top: 0, left: 8, right: 8, height: '4px', cursor: 'n-resize', zIndex: 1 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 's')} style={{ position: 'absolute', bottom: 0, left: 8, right: 8, height: '4px', cursor: 's-resize', zIndex: 1 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 'w')} style={{ position: 'absolute', top: 8, left: 0, bottom: 8, width: '4px', cursor: 'w-resize', zIndex: 1 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 'e')} style={{ position: 'absolute', top: 8, right: 0, bottom: 8, width: '4px', cursor: 'e-resize', zIndex: 1 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 'nw')} style={{ position: 'absolute', top: 0, left: 0, width: '8px', height: '8px', cursor: 'nw-resize', zIndex: 2 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 'ne')} style={{ position: 'absolute', top: 0, right: 0, width: '8px', height: '8px', cursor: 'ne-resize', zIndex: 2 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 'sw')} style={{ position: 'absolute', bottom: 0, left: 0, width: '8px', height: '8px', cursor: 'sw-resize', zIndex: 2 }} />
          <div onMouseDown={e => handleResizeMouseDown(e, 'se')} style={{ position: 'absolute', bottom: 0, right: 0, width: '8px', height: '8px', cursor: 'se-resize', zIndex: 2 }} />
        </>
      )}
    </div>
  );
}

// ============ TASKBAR ============
function Taskbar({
  windows,
  startMenuOpen,
  onToggleStartMenu,
  onWindowClick,
  activeWindowId,
}: {
  windows: WindowState[];
  startMenuOpen: boolean;
  onToggleStartMenu: () => void;
  onWindowClick: (id: string) => void;
  activeWindowId: string | null;
}) {
  const [time, setTime] = useState(new Date());

  useEffect(() => {
    const timer = setInterval(() => setTime(new Date()), 1000);
    return () => clearInterval(timer);
  }, []);

  return (
    <div style={{
      position: 'absolute', bottom: 0, left: 0, right: 0, height: '48px',
      background: 'rgba(24, 24, 27, 0.95)', backdropFilter: 'blur(12px)',
      borderTop: '1px solid #333', display: 'flex', alignItems: 'center',
      padding: '0 8px', zIndex: 9000,
    }}>
      {/* Start button */}
      <button
        onClick={onToggleStartMenu}
        style={{
          padding: '6px 14px', background: startMenuOpen ? '#3c3c3c' : 'transparent',
          border: '1px solid ' + (startMenuOpen ? '#555' : '#3f3f46'), color: '#fafafa',
          fontSize: '13px', cursor: 'pointer', borderRadius: '6px',
          fontFamily: 'inherit', fontWeight: 600, transition: 'all 0.15s',
          display: 'flex', alignItems: 'center', gap: '6px',
        }}
        onMouseEnter={e => (e.currentTarget.style.background = '#333')}
        onMouseLeave={e => { if (!startMenuOpen) e.currentTarget.style.background = 'transparent'; }}
      >
        🖥️ WebOS
      </button>

      {/* Running apps */}
      <div style={{ flex: 1, display: 'flex', alignItems: 'center', gap: '4px', padding: '0 8px', overflow: 'hidden' }}>
        {windows.map(w => (
          <button
            key={w.id}
            onClick={() => onWindowClick(w.id)}
            style={{
              padding: '4px 12px', background: w.id === activeWindowId ? '#3c3c3c' : 'transparent',
              border: '1px solid ' + (w.id === activeWindowId ? '#555' : '#3f3f46'),
              color: w.isMinimized ? '#888' : '#fafafa', fontSize: '12px', cursor: 'pointer',
              borderRadius: '4px', fontFamily: 'inherit', whiteSpace: 'nowrap',
              maxWidth: '160px', overflow: 'hidden', textOverflow: 'ellipsis',
              transition: 'all 0.15s',
            }}
            onMouseEnter={e => (e.currentTarget.style.background = '#333')}
            onMouseLeave={e => { if (w.id !== activeWindowId) e.currentTarget.style.background = 'transparent'; }}
          >
            {w.appId === 'terminal' ? '⬛' : w.appId === 'filemanager' ? '📁' : '⚙️'} {w.title}
          </button>
        ))}
      </div>

      {/* Clock */}
      <div style={{ fontSize: '12px', color: '#a1a1aa', fontFamily: "'SF Mono', 'Fira Code', Consolas, monospace", padding: '0 8px', whiteSpace: 'nowrap' }}>
        {time.toLocaleTimeString('zh-CN', { hour: '2-digit', minute: '2-digit' })}
      </div>
    </div>
  );
}

// ============ START MENU ============
function StartMenu({ onLaunchApp, onClose }: { onLaunchApp: (appId: string) => void; onClose: () => void }) {
  const apps = [
    { id: 'terminal', name: '终端', icon: '⬛', desc: '命令行终端' },
    { id: 'filemanager', name: '文件管理器', icon: '📁', desc: '浏览文件系统' },
    { id: 'settings', name: '设置', icon: '⚙️', desc: '系统设置' },
  ];

  return (
    <>
      {/* Backdrop */}
      <div style={{ position: 'fixed', inset: 0, zIndex: 8999 }} onClick={onClose} />
      <div style={{
        position: 'absolute', bottom: '56px', left: '8px', width: '260px',
        background: 'rgba(30, 30, 33, 0.98)', backdropFilter: 'blur(16px)',
        border: '1px solid #454545', borderRadius: '10px', padding: '8px',
        zIndex: 9001, boxShadow: '0 8px 32px rgba(0,0,0,0.5)',
      }}>
        <div style={{ padding: '8px 12px 12px', borderBottom: '1px solid #333', marginBottom: '4px' }}>
          <div style={{ fontSize: '14px', fontWeight: 600, color: '#fff' }}>WebOS</div>
          <div style={{ fontSize: '11px', color: '#666' }}>v0.0.1beta</div>
        </div>
        {apps.map(app => (
          <button
            key={app.id}
            onClick={() => { onLaunchApp(app.id); onClose(); }}
            style={{
              display: 'flex', alignItems: 'center', gap: '12px', width: '100%', padding: '10px 12px',
              background: 'none', border: 'none', color: '#ccc', cursor: 'pointer',
              borderRadius: '6px', transition: 'background 0.15s', textAlign: 'left',
            }}
            onMouseEnter={e => (e.currentTarget.style.background = '#333')}
            onMouseLeave={e => (e.currentTarget.style.background = 'none')}
          >
            <span style={{ fontSize: '22px' }}>{app.icon}</span>
            <div>
              <div style={{ fontSize: '13px', fontWeight: 500, color: '#fff' }}>{app.name}</div>
              <div style={{ fontSize: '11px', color: '#888' }}>{app.desc}</div>
            </div>
          </button>
        ))}
      </div>
    </>
  );
}

// ============ DESKTOP ============
function Desktop({ fs }: { fs: FileSystem }) {
  const [windows, setWindows] = useState<WindowState[]>([]);
  const [nextZIndex, setNextZIndex] = useState(100);
  const [startMenuOpen, setStartMenuOpen] = useState(false);
  const [wallpaper, setWallpaper] = useState('linear-gradient(135deg, #0c0015, #1a0033, #0a1628)');
  const [activeWindowId, setActiveWindowId] = useState<string | null>(null);

  // Load wallpaper from localStorage
  useEffect(() => {
    try {
      const saved = localStorage.getItem('webos-wallpaper');
      if (saved) setWallpaper(saved);
    } catch { /* ignore */ }
  }, []);

  const handleWallpaperChange = useCallback((wp: string) => {
    setWallpaper(wp);
    try { localStorage.setItem('webos-wallpaper', wp); } catch { /* ignore */ }
  }, []);

  const windowCounters = useRef({ terminal: 0, filemanager: 0, settings: 0 });

  const launchApp = useCallback((appId: string) => {
    const counters = windowCounters.current;
    counters[appId as keyof typeof counters] = (counters[appId as keyof typeof counters] || 0) + 1;
    const count = counters[appId as keyof typeof counters];

    const appConfigs: { [key: string]: { title: string; width: number; height: number; minWidth: number; minHeight: number } } = {
      terminal: { title: `终端 #${count}`, width: 680, height: 440, minWidth: 400, minHeight: 250 },
      filemanager: { title: `文件管理器 #${count}`, width: 720, height: 500, minWidth: 450, minHeight: 300 },
      settings: { title: `设置 #${count}`, width: 520, height: 480, minWidth: 350, minHeight: 300 },
    };

    const config = appConfigs[appId] || appConfigs.terminal;
    const id = `${appId}-${Date.now()}-${Math.random().toString(36).slice(2, 6)}`;
    const offset = (windows.length % 8) * 30;

    const newWindow: WindowState = {
      id,
      appId,
      title: config.title,
      x: 80 + offset,
      y: 40 + offset,
      width: config.width,
      height: config.height,
      minWidth: config.minWidth,
      minHeight: config.minHeight,
      isMinimized: false,
      isMaximized: false,
      zIndex: nextZIndex,
    };

    setNextZIndex(prev => prev + 1);
    setWindows(prev => [...prev, newWindow]);
    setActiveWindowId(id);
  }, [windows.length, nextZIndex]);

  const closeWindow = useCallback((id: string) => {
    setWindows(prev => prev.filter(w => w.id !== id));
    setActiveWindowId(prev => prev === id ? null : prev);
  }, []);

  const minimizeWindow = useCallback((id: string) => {
    setWindows(prev => prev.map(w => w.id === id ? { ...w, isMinimized: !w.isMinimized } : w));
  }, []);

  const maximizeWindow = useCallback((id: string) => {
    setWindows(prev => prev.map(w => {
      if (w.id !== id) return w;
      if (w.isMaximized) {
        return {
          ...w,
          isMaximized: false,
          x: w.prevBounds?.x ?? 80,
          y: w.prevBounds?.y ?? 40,
          width: w.prevBounds?.width ?? 680,
          height: w.prevBounds?.height ?? 440,
          prevBounds: undefined,
        };
      } else {
        return {
          ...w,
          isMaximized: true,
          prevBounds: { x: w.x, y: w.y, width: w.width, height: w.height },
        };
      }
    }));
  }, []);

  const focusWindow = useCallback((id: string) => {
    setNextZIndex(prev => {
      setWindows(ws => ws.map(w => w.id === id ? { ...w, zIndex: prev } : w));
      return prev + 1;
    });
    setActiveWindowId(id);
    setStartMenuOpen(false);
  }, []);

  const moveWindow = useCallback((id: string, x: number, y: number) => {
    setWindows(prev => prev.map(w => w.id === id ? { ...w, x, y } : w));
  }, []);

  const resizeWindow = useCallback((id: string, width: number, height: number) => {
    setWindows(prev => prev.map(w => w.id === id ? { ...w, width, height } : w));
  }, []);

  const handleWindowClick = useCallback((id: string) => {
    const w = windows.find(w => w.id === id);
    if (w?.isMinimized) {
      setWindows(prev => prev.map(w => w.id === id ? { ...w, isMinimized: false } : w));
    }
    focusWindow(id);
  }, [windows, focusWindow]);

  const desktopIcons = [
    { appId: 'terminal', name: '终端', icon: '⬛' },
    { appId: 'filemanager', name: '文件管理器', icon: '📁' },
    { appId: 'settings', name: '设置', icon: '⚙️' },
  ];

  return (
    <div style={{
      width: '100%', height: '100vh', position: 'relative', overflow: 'hidden',
      background: wallpaper, userSelect: 'none',
    }}>
      {/* Desktop Icons */}
      <div style={{ position: 'absolute', top: '16px', left: '16px', display: 'flex', flexDirection: 'column', gap: '8px', zIndex: 1 }}>
        {desktopIcons.map(icon => (
          <div
            key={icon.appId}
            onDoubleClick={() => launchApp(icon.appId)}
            style={{
              display: 'flex', flexDirection: 'column', alignItems: 'center', justifyContent: 'center',
              width: '76px', height: '76px', borderRadius: '8px', cursor: 'pointer', padding: '8px 4px',
              transition: 'background 0.15s',
            }}
            onMouseEnter={e => (e.currentTarget.style.background = 'rgba(255,255,255,0.08)')}
            onMouseLeave={e => (e.currentTarget.style.background = 'transparent')}
          >
            <span style={{ fontSize: '32px', lineHeight: 1, filter: 'drop-shadow(0 2px 4px rgba(0,0,0,0.3))' }}>{icon.icon}</span>
            <span style={{ fontSize: '11px', color: '#e0e0e0', marginTop: '6px', textShadow: '0 1px 3px rgba(0,0,0,0.8)', textAlign: 'center', lineHeight: 1.2 }}>{icon.name}</span>
          </div>
        ))}
      </div>

      {/* Windows */}
      {windows.map(w => (
        <Window
          key={w.id}
          windowState={w}
          isActive={w.id === activeWindowId}
          onClose={() => closeWindow(w.id)}
          onMinimize={() => minimizeWindow(w.id)}
          onMaximize={() => maximizeWindow(w.id)}
          onFocus={() => focusWindow(w.id)}
          onMove={(x, y) => moveWindow(w.id, x, y)}
          onResize={(width, height) => resizeWindow(w.id, width, height)}
        >
          {w.appId === 'terminal' && <TerminalApp fs={fs} initialCwd="/home/user" onCwdChange={() => {}} />}
          {w.appId === 'filemanager' && <FileManagerApp fs={fs} />}
          {w.appId === 'settings' && <SettingsApp wallpaper={wallpaper} onWallpaperChange={handleWallpaperChange} />}
        </Window>
      ))}

      {/* Start Menu */}
      {startMenuOpen && (
        <StartMenu
          onLaunchApp={launchApp}
          onClose={() => setStartMenuOpen(false)}
        />
      )}

      {/* Taskbar */}
      <Taskbar
        windows={windows}
        startMenuOpen={startMenuOpen}
        onToggleStartMenu={() => setStartMenuOpen(prev => !prev)}
        onWindowClick={handleWindowClick}
        activeWindowId={activeWindowId}
      />
    </div>
  );
}

// ============ MAIN OS PAGE ============
export default function OSPage() {
  const [booting, setBooting] = useState(true);
  const [fs, setFs] = useState<FileSystem | null>(null);

  const handleBootComplete = useCallback(() => {
    const fileSystem = new FileSystem();
    setFs(fileSystem);
    setBooting(false);
  }, []);

  if (booting) {
    return <BootScreen onComplete={handleBootComplete} />;
  }

  if (!fs) return null;

  return <Desktop fs={fs} />;
}
