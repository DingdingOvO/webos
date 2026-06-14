'use client';

import React, { memo } from 'react';
import Link from 'next/link';

type Lang = 'zh' | 'zh-TW' | 'en' | 'ja' | 'ko' | 'ru';

interface NavbarProps {
  active?: 'intro' | 'docs' | 'app';
  lang: Lang;
  onLangChange: (lang: Lang) => void;
  theme: 'light' | 'dark';
  onThemeChange: () => void;
  onMenuClick?: () => void;
  isMobile?: boolean;
}

const NAV_I18N: Record<Lang, { intro: string; docs: string; app: string; github: string }> = {
  zh: { intro: '介绍', docs: '文档', app: '启动', github: 'GitHub' },
  'zh-TW': { intro: '介紹', docs: '文檔', app: '啟動', github: 'GitHub' },
  en: { intro: 'Intro', docs: 'Docs', app: 'Launch', github: 'GitHub' },
  ja: { intro: '紹介', docs: 'ドキュメント', app: '起動', github: 'GitHub' },
  ko: { intro: '소개', docs: '문서', app: '실행', github: 'GitHub' },
  ru: { intro: 'Введение', docs: 'Документация', app: 'Запуск', github: 'GitHub' },
};

const SunIcon = memo(() => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="18" height="18">
    <circle cx="12" cy="12" r="4" />
    <path d="M12 2v2M12 20v2M4.93 4.93l1.41 1.41M17.66 17.66l1.41 1.41M2 12h2M20 12h2M6.34 17.66l-1.41 1.41M19.07 4.93l-1.41 1.41" />
  </svg>
));

const MoonIcon = memo(() => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="18" height="18">
    <path d="M21 12.79A9 9 0 1111.21 3 7 7 0 0021 12.79z" />
  </svg>
));

const GitHubIcon = memo(() => (
  <svg viewBox="0 0 24 24" fill="currentColor" width="18" height="18">
    <path d="M12 0C5.37 0 0 5.37 0 12c0 5.31 3.435 9.795 8.205 11.385.6.105.825-.255.825-.57 0-.285-.015-1.23-.015-2.235-3.015.555-3.795-.735-4.035-1.41-.135-.345-.72-1.41-1.23-1.695-.42-.225-1.02-.78-.015-.795.945-.015 1.62.87 1.845 1.23 1.08 1.815 2.805 1.305 3.495.99.105-.78.42-1.305.765-1.605-2.67-.3-5.46-1.335-5.46-5.925 0-1.305.465-2.385 1.23-3.225-.12-.3-.54-1.53.12-3.18 0 0 1.005-.315 3.3 1.23.96-.27 1.98-.405 3-.405s2.04.135 3 .405c2.295-1.56 3.3-1.23 3.3-1.23.66 1.65.24 2.88.12 3.18.765.84 1.23 1.905 1.23 3.225 0 4.605-2.805 5.625-5.475 5.925.435.375.81 1.095.81 2.22 0 1.605-.015 2.895-.015 3.3 0 .315.225.69.825.57A12.02 12.02 0 0024 12c0-6.63-5.37-12-12-12z" />
  </svg>
));

const MenuIcon = memo(() => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="1.5" width="20" height="20">
    <path d="M4 6h16M4 12h16M4 18h16" />
  </svg>
));

export default function Navbar({
  active,
  lang,
  onLangChange,
  theme,
  onThemeChange,
  onMenuClick,
  isMobile,
}: NavbarProps) {
  const t = NAV_I18N[lang];
  const isDark = theme === 'dark';

  return (
    <header className="navbar">
      <div className="navbar-inner">
        <div className="navbar-left">
          {isMobile && onMenuClick && (
            <button className="icon-btn" onClick={onMenuClick} aria-label="菜单">
              <MenuIcon />
            </button>
          )}
          <Link href="/intro" className="navbar-logo">
            WebOS
          </Link>
          {!isMobile && (
            <nav className="navbar-links">
              <Link href="/intro" className={active === 'intro' ? 'active' : ''}>
                {t.intro}
              </Link>
              <Link href="/docs" className={active === 'docs' ? 'active' : ''}>
                {t.docs}
              </Link>
              <Link href="/app/os" className={active === 'app' ? 'active' : ''}>
                {t.app}
              </Link>
            </nav>
          )}
        </div>
        <div className="navbar-right">
          <a
            href="https://github.com/DingdingOvO/webos"
            target="_blank"
            rel="noopener noreferrer"
            className="icon-btn"
            title={t.github}
          >
            <GitHubIcon />
          </a>
          <button onClick={onThemeChange} className="icon-btn" aria-label="切换主题">
            {isDark ? <SunIcon /> : <MoonIcon />}
          </button>
          <select
            value={lang}
            onChange={(e) => onLangChange(e.target.value as Lang)}
            className="lang-select"
          >
            <option value="zh">简体中文</option>
            <option value="zh-TW">繁體中文</option>
            <option value="en">English</option>
            <option value="ja">日本語</option>
            <option value="ko">한국어</option>
            <option value="ru">Русский</option>
          </select>
        </div>
      </div>
    </header>
  );
}
