'use client';

import { useState, useEffect } from 'react';
import Navbar from '../../components/Navbar';
import { useTheme, Lang } from '../../components/useTheme';

const i18n = {
  zh: {
    hero: {
      badge: 'v0.0.1beta',
      title: 'WebOS',
      subtitle: '浏览器中的操作系统',
      desc: '用 C/C++ 编写内核，编译为 WebAssembly，WebGPU 渲染每一帧画面。不是模拟器，不是主题——是真正的操作系统在浏览器中运行。',
      primary: '在线体验',
      secondary: '查看文档',
    },
    features: [
      { icon: 'gpu', title: 'WebGPU 渲染', desc: '所有像素自己画。F12 只看到画布，没有 DOM，没有 XSS 风险' },
      { icon: 'wasm', title: 'C/C++ + WASM', desc: '内核、驱动、服务全用 C 写，.wex/.Wdll/.wli 模块架构' },
      { icon: 'fs', title: '真实文件系统', desc: 'WebFS — superblock、128B inode、4KB 数据块、IndexedDB 后端' },
      { icon: 'audio', title: '统一音频', desc: '音频服务器 + 混合器，多流并发，tanh 软削波' },
    ],
    apps: {
      title: '10 个预装应用',
      items: ['Terminal', 'File Manager', 'Text Editor', 'Settings', 'Calculator', 'Paint', 'Music Player', 'System Monitor', 'Browser', 'App Store'],
    },
    boot: {
      title: '启动链',
      items: ['TypeScript Launcher', 'C Bootloader', 'C Kernel', 'Login System', 'Desktop'],
    },
    tech: { title: '技术栈', items: ['C / C++', 'WebAssembly', 'WebGPU', 'Emscripten', 'TypeScript'] },
    footer: 'WebOS v0.0.1beta · MIT License',
  },
  en: {
    hero: {
      badge: 'v0.0.1beta',
      title: 'WebOS',
      subtitle: 'A Real OS in Your Browser',
      desc: 'C/C++ kernel compiled to WebAssembly, WebGPU renders every pixel. Not an emulator, not a theme — a real operating system running in the browser.',
      primary: 'Try Online',
      secondary: 'Read Docs',
    },
    features: [
      { icon: 'gpu', title: 'WebGPU Rendered', desc: 'Every pixel painted by us. F12 shows only a canvas — no DOM, no XSS' },
      { icon: 'wasm', title: 'C/C++ + WASM', desc: 'Kernel, drivers, services in C. .wex/.Wdll/.wli module architecture' },
      { icon: 'fs', title: 'Real Filesystem', desc: 'WebFS — superblock, 128B inodes, 4KB data blocks, IndexedDB backend' },
      { icon: 'audio', title: 'Unified Audio', desc: 'Audio server + mixer, multi-stream, tanh soft clipping' },
    ],
    apps: {
      title: '10 Pre-installed Apps',
      items: ['Terminal', 'File Manager', 'Text Editor', 'Settings', 'Calculator', 'Paint', 'Music Player', 'System Monitor', 'Browser', 'App Store'],
    },
    boot: {
      title: 'Boot Chain',
      items: ['TypeScript Launcher', 'C Bootloader', 'C Kernel', 'Login System', 'Desktop'],
    },
    tech: { title: 'Tech Stack', items: ['C / C++', 'WebAssembly', 'WebGPU', 'Emscripten', 'TypeScript'] },
    footer: 'WebOS v0.0.1beta · MIT License',
  },
} as const;

type LangKey = 'zh' | 'en';

const GpuIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="28" height="28">
    <rect x="2" y="3" width="20" height="14" rx="2" />
    <path d="M8 21h8M12 17v4" />
    <circle cx="12" cy="10" r="3" />
  </svg>
);

const WasmIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="28" height="28">
    <path d="M21 16V8a2 2 0 00-1-1.73l-7-4a2 2 0 00-2 0l-7 4A2 2 0 003 8v8a2 2 0 001 1.73l7 4a2 2 0 002 0l7-4A2 2 0 0021 16z" />
    <path d="M3.27 6.96L12 12.01l8.73-5.05M12 22.08V12" />
  </svg>
);

const FsIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="28" height="28">
    <path d="M3 7v10a2 2 0 002 2h14a2 2 0 002-2V9a2 2 0 00-2-2h-6l-2-2H5a2 2 0 00-2 2z" />
    <path d="M8 13h8" />
  </svg>
);

const AudioIcon = () => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="28" height="28">
    <path d="M11 5L6 9H2v6h4l5 4V5z" />
    <path d="M19.07 4.93a10 10 0 010 14.14M15.54 8.46a5 5 0 010 7.07" />
  </svg>
);

const iconMap: Record<string, React.ReactNode> = {
  gpu: <GpuIcon />,
  wasm: <WasmIcon />,
  fs: <FsIcon />,
  audio: <AudioIcon />,
};

const appIcons = ['💻', '📁', '📝', '⚙️', '🔢', '🎨', '🎵', '📊', '🌐', '🏪'];

export default function IntroPage() {
  const { lang, theme, mounted, setLang, toggleTheme, isDark } = useTheme();
  const [isMobile, setIsMobile] = useState(false);
  const t = i18n[(lang === 'zh' || lang === 'zh-TW') ? 'zh' : 'en'] || i18n.en;

  useEffect(() => {
    const check = () => setIsMobile(window.innerWidth < 768);
    check();
    window.addEventListener('resize', check);
    return () => window.removeEventListener('resize', check);
  }, []);

  if (!mounted) {
    return <div style={{ minHeight: '100vh', background: 'var(--bg)' }} />;
  }

  return (
    <div style={{ minHeight: '100vh', display: 'flex', flexDirection: 'column' }}>
      <Navbar
        active="intro"
        lang={lang}
        onLangChange={setLang}
        theme={theme}
        onThemeChange={toggleTheme}
        isMobile={isMobile}
      />

      <main style={{ flex: 1, background: 'var(--bg-gradient)' }}>
        {/* Hero */}
        <section className="hero-section">
          <div className="hero-badge">{t.hero.badge}</div>
          <h1 className="hero-title">{t.hero.title}</h1>
          <p className="hero-subtitle">{t.hero.subtitle}</p>
          <p className="hero-desc">{t.hero.desc}</p>
          <div className="hero-actions">
            <a href="/app/os" className="btn-primary">
              {t.hero.primary}
            </a>
            <a href="/docs" className="btn-secondary">
              {t.hero.secondary}
            </a>
          </div>
        </section>

        {/* Features */}
        <section className="features-grid">
          {t.features.map((feature, i) => (
            <div key={i} className="feature-card">
              <div className="feature-icon">{iconMap[feature.icon]}</div>
              <h3>{feature.title}</h3>
              <p>{feature.desc}</p>
            </div>
          ))}
        </section>

        {/* Pre-installed Apps */}
        <section style={{ maxWidth: 800, margin: '0 auto', padding: '0 24px 64px', textAlign: 'center' }}>
          <h2 style={{ fontSize: 20, fontWeight: 600, marginBottom: 24, color: 'var(--text)' }}>
            {t.apps.title}
          </h2>
          <div style={{ display: 'flex', flexWrap: 'wrap', justifyContent: 'center', gap: 12 }}>
            {t.apps.items.map((app, i) => (
              <span key={i} style={{
                display: 'inline-flex',
                alignItems: 'center',
                gap: 6,
                padding: '8px 14px',
                background: 'var(--surface)',
                border: '1px solid var(--border)',
                borderRadius: 6,
                fontSize: 13,
                color: 'var(--text-secondary)',
              }}>
                <span>{appIcons[i]}</span>
                {app}
              </span>
            ))}
          </div>
        </section>

        {/* Boot Chain */}
        <section style={{ maxWidth: 800, margin: '0 auto', padding: '0 24px 64px', textAlign: 'center' }}>
          <h2 style={{ fontSize: 20, fontWeight: 600, marginBottom: 24, color: 'var(--text)' }}>
            {t.boot.title}
          </h2>
          <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'center', gap: 8, flexWrap: 'wrap' }}>
            {t.boot.items.map((item, i) => (
              <span key={i} style={{ display: 'inline-flex', alignItems: 'center', gap: 8 }}>
                <span style={{
                  padding: '8px 16px',
                  background: 'var(--surface)',
                  border: '1px solid var(--border)',
                  borderRadius: 6,
                  fontSize: 13,
                  fontWeight: 500,
                  color: i === 2 ? '#3B82F6' : 'var(--text-secondary)',
                }}>
                  {item}
                </span>
                {i < t.boot.items.length - 1 && (
                  <span style={{ color: 'var(--text-muted)', fontSize: 16 }}>→</span>
                )}
              </span>
            ))}
          </div>
        </section>

        {/* Tech Stack */}
        <section className="tech-section">
          <p className="tech-label">{t.tech.title}</p>
          <div className="tech-tags">
            {t.tech.items.map((item, i) => (
              <span key={i} className="tech-tag">{item}</span>
            ))}
          </div>
        </section>
      </main>

      <footer className="page-footer">
        <p>{t.footer}</p>
      </footer>
    </div>
  );
}
