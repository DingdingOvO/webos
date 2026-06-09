'use client';

import { useState, useEffect, useRef } from 'react';
import { useTheme, Lang } from './useTheme';

const NAV_I18N: Record<Lang, { intro: string; docs: string; launch: string; github: string; lang: string }> = {
  zh: { intro: '介绍', docs: '文档', launch: '启动', github: 'GitHub', lang: '简体中文' },
  'zh-TW': { intro: '介紹', docs: '文檔', launch: '啟動', github: 'GitHub', lang: '繁體中文' },
  en: { intro: 'Intro', docs: 'Docs', launch: 'Launch', github: 'GitHub', lang: 'English' },
  ja: { intro: '紹介', docs: 'ドキュメント', launch: '起動', github: 'GitHub', lang: '日本語' },
  ko: { intro: '소개', docs: '문서', launch: '실행', github: 'GitHub', lang: '한국어' },
  ru: { intro: 'Введение', docs: 'Документация', launch: 'Запуск', github: 'GitHub', lang: 'Русский' },
};

const LANG_OPTIONS: { value: Lang; label: string }[] = [
  { value: 'zh', label: '简体中文' },
  { value: 'zh-TW', label: '繁體中文' },
  { value: 'en', label: 'English' },
  { value: 'ja', label: '日本語' },
  { value: 'ko', label: '한국어' },
  { value: 'ru', label: 'Русский' },
];

interface NavbarProps {
  lang: Lang;
  setLang: (lang: Lang) => void;
  theme: 'light' | 'dark';
  toggleTheme: () => void;
  mounted: boolean;
}

export default function Navbar({ lang, setLang, theme, toggleTheme, mounted }: NavbarProps) {
  const [mobileOpen, setMobileOpen] = useState(false);
  const [langOpen, setLangOpen] = useState(false);
  const langRef = useRef<HTMLDivElement>(null);

  const t = NAV_I18N[lang];

  useEffect(() => {
    function handleClickOutside(e: MouseEvent) {
      if (langRef.current && !langRef.current.contains(e.target as Node)) {
        setLangOpen(false);
      }
    }
    document.addEventListener('mousedown', handleClickOutside);
    return () => document.removeEventListener('mousedown', handleClickOutside);
  }, []);

  const handleLangSelect = (newLang: Lang) => {
    setLang(newLang);
    setLangOpen(false);
  };

  if (!mounted) {
    return (
      <nav className="navbar">
        <div className="navbar-inner">
          <a href="/intro" className="navbar-logo">WebOS</a>
          <div className="navbar-links">
            <a href="/intro">Intro</a>
            <a href="/docs">Docs</a>
            <a href="/app/os">Launch</a>
          </div>
          <div className="navbar-actions">
            <div style={{ width: 36, height: 36 }} />
          </div>
        </div>
      </nav>
    );
  }

  return (
    <>
      <nav className="navbar">
        <div className="navbar-inner">
          <a href="/intro" className="navbar-logo">WebOS</a>

          <div className="navbar-links">
            <a href="/intro">{t.intro}</a>
            <a href="/docs">{t.docs}</a>
            <a href="/app/os">{t.launch}</a>
          </div>

          <div className="navbar-actions">
            {/* Theme toggle */}
            <button
              className="navbar-btn"
              onClick={toggleTheme}
              aria-label={theme === 'dark' ? 'Switch to light mode' : 'Switch to dark mode'}
              title={theme === 'dark' ? 'Light mode' : 'Dark mode'}
            >
              {theme === 'dark' ? (
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                  <circle cx="12" cy="12" r="5" />
                  <line x1="12" y1="1" x2="12" y2="3" />
                  <line x1="12" y1="21" x2="12" y2="23" />
                  <line x1="4.22" y1="4.22" x2="5.64" y2="5.64" />
                  <line x1="18.36" y1="18.36" x2="19.78" y2="19.78" />
                  <line x1="1" y1="12" x2="3" y2="12" />
                  <line x1="21" y1="12" x2="23" y2="12" />
                  <line x1="4.22" y1="19.78" x2="5.64" y2="18.36" />
                  <line x1="18.36" y1="5.64" x2="19.78" y2="4.22" />
                </svg>
              ) : (
                <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                  <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" />
                </svg>
              )}
            </button>

            {/* Language selector */}
            <div className="lang-selector" ref={langRef}>
              <button
                className="lang-btn"
                onClick={() => setLangOpen(!langOpen)}
                aria-label="Select language"
              >
                <svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                  <circle cx="12" cy="12" r="10" />
                  <line x1="2" y1="12" x2="22" y2="12" />
                  <path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z" />
                </svg>
                {lang.toUpperCase()}
              </button>
              {langOpen && (
                <div className="lang-dropdown">
                  {LANG_OPTIONS.map((opt) => (
                    <button
                      key={opt.value}
                      className={`lang-option ${lang === opt.value ? 'active' : ''}`}
                      onClick={() => handleLangSelect(opt.value)}
                    >
                      {opt.label}
                    </button>
                  ))}
                </div>
              )}
            </div>

            {/* GitHub link */}
            <a
              href="https://github.com/DingdingOvO/webos"
              target="_blank"
              rel="noopener noreferrer"
              className="navbar-btn"
              aria-label="GitHub"
              title="GitHub"
              style={{ textDecoration: 'none' }}
            >
              <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">
                <path d="M12 0c-6.626 0-12 5.373-12 12 0 5.302 3.438 9.8 8.207 11.387.599.111.793-.261.793-.577v-2.234c-3.338.726-4.033-1.416-4.033-1.416-.546-1.387-1.333-1.756-1.333-1.756-1.089-.745.083-.729.083-.729 1.205.084 1.839 1.237 1.839 1.237 1.07 1.834 2.807 1.304 3.492.997.107-.775.418-1.305.762-1.604-2.665-.305-5.467-1.334-5.467-5.931 0-1.311.469-2.381 1.236-3.221-.124-.303-.535-1.524.117-3.176 0 0 1.008-.322 3.301 1.23.957-.266 1.983-.399 3.003-.404 1.02.005 2.047.138 3.006.404 2.291-1.552 3.297-1.23 3.297-1.23.653 1.653.242 2.874.118 3.176.77.84 1.235 1.911 1.235 3.221 0 4.609-2.807 5.624-5.479 5.921.43.372.823 1.102.823 2.222v3.293c0 .319.192.694.801.576 4.765-1.589 8.199-6.086 8.199-11.386 0-6.627-5.373-12-12-12z" />
              </svg>
            </a>

            {/* Mobile menu button */}
            <button
              className="mobile-menu-btn"
              onClick={() => setMobileOpen(!mobileOpen)}
              aria-label="Toggle menu"
            >
              {mobileOpen ? (
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                  <line x1="18" y1="6" x2="6" y2="18" />
                  <line x1="6" y1="6" x2="18" y2="18" />
                </svg>
              ) : (
                <svg width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                  <line x1="3" y1="12" x2="21" y2="12" />
                  <line x1="3" y1="6" x2="21" y2="6" />
                  <line x1="3" y1="18" x2="21" y2="18" />
                </svg>
              )}
            </button>
          </div>
        </div>
      </nav>

      {/* Mobile menu */}
      <div className={`mobile-menu ${mobileOpen ? 'open' : ''}`}>
        <a href="/intro" onClick={() => setMobileOpen(false)}>{t.intro}</a>
        <a href="/docs" onClick={() => setMobileOpen(false)}>{t.docs}</a>
        <a href="/app/os" onClick={() => setMobileOpen(false)}>{t.launch}</a>
        <div className="mobile-menu-actions">
          <button
            className="navbar-btn"
            onClick={() => { toggleTheme(); setMobileOpen(false); }}
            aria-label="Toggle theme"
          >
            {theme === 'dark' ? (
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <circle cx="12" cy="12" r="5" />
                <line x1="12" y1="1" x2="12" y2="3" />
                <line x1="12" y1="21" x2="12" y2="23" />
                <line x1="4.22" y1="4.22" x2="5.64" y2="5.64" />
                <line x1="18.36" y1="18.36" x2="19.78" y2="19.78" />
                <line x1="1" y1="12" x2="3" y2="12" />
                <line x1="21" y1="12" x2="23" y2="12" />
                <line x1="4.22" y1="19.78" x2="5.64" y2="18.36" />
                <line x1="18.36" y1="5.64" x2="19.78" y2="4.22" />
              </svg>
            ) : (
              <svg width="18" height="18" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <path d="M21 12.79A9 9 0 1 1 11.21 3 7 7 0 0 0 21 12.79z" />
              </svg>
            )}
          </button>
          {LANG_OPTIONS.map((opt) => (
            <button
              key={opt.value}
              className={`lang-option ${lang === opt.value ? 'active' : ''}`}
              onClick={() => { handleLangSelect(opt.value); setMobileOpen(false); }}
              style={{ border: '1px solid var(--border)', padding: '0.25rem 0.5rem', borderRadius: 'var(--radius-sm)', fontSize: '0.8rem' }}
            >
              {opt.label}
            </button>
          ))}
        </div>
      </div>
    </>
  );
}
