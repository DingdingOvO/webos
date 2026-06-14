'use client';

import React, { useState, useEffect, useCallback, memo } from 'react';
import Link from 'next/link';
import { usePathname } from 'next/navigation';
import Navbar from '../../components/Navbar';
import { useTheme, Lang } from '../../components/useTheme';

// ============================================================================
// Sidebar i18n
// ============================================================================
const SIDEBAR_I18N: Record<Lang, Record<string, string>> = {
  zh: {
    'architecture': '系统架构',
    'module_spec': '模块格式规范',
    'building': '构建与运行',
    'api_syscall': '系统调用 API',
    'api_services': '系统服务 API',
    'developer_guide': '应用开发指南',
    'app_store': '应用商店规范',
    'iso_build': 'ISO 构建',
    'dev-plan/00_总览': '📋 项目总览',
    'dev-plan/01_设计语言': '🎨 设计语言',
    'dev-plan/02_内核': '🔧 内核',
    'dev-plan/03_文件系统': '📁 文件系统',
    'dev-plan/04_音频系统': '🔊 音频系统',
    'dev-plan/05_驱动层': '🔌 驱动层',
    'dev-plan/06_桌面环境': '🖥 桌面环境',
    'dev-plan/07_应用生态': '📱 应用生态',
    'dev-plan/08_安全特性': '🔒 安全特性',
    'dev-plan/09_模块系统': '📦 模块系统',
    'dev-plan/10_ISO与网站': '🌐 ISO与网站',
  },
  'zh-TW': {
    'architecture': '系統架構',
    'module_spec': '模組格式規範',
    'building': '建置與執行',
    'api_syscall': '系統呼叫 API',
    'api_services': '系統服務 API',
    'developer_guide': '應用開發指南',
    'app_store': '應用商店規範',
    'iso_build': 'ISO 建置',
    'dev-plan/00_总览': '📋 項目總覽',
    'dev-plan/01_设计语言': '🎨 設計語言',
    'dev-plan/02_内核': '🔧 內核',
    'dev-plan/03_文件系统': '📁 檔案系統',
    'dev-plan/04_音频系统': '🔊 音頻系統',
    'dev-plan/05_驱动层': '🔌 驅動層',
    'dev-plan/06_桌面环境': '🖥 桌面環境',
    'dev-plan/07_应用生态': '📱 應用生態',
    'dev-plan/08_安全特性': '🔒 安全特性',
    'dev-plan/09_模块系统': '📦 模組系統',
    'dev-plan/10_ISO与网站': '🌐 ISO與網站',
  },
  en: {
    'architecture': 'System Architecture',
    'module_spec': 'Module Specification',
    'building': 'Building & Running',
    'api_syscall': 'Syscall API',
    'api_services': 'Services API',
    'developer_guide': 'Developer Guide',
    'app_store': 'App Store Specification',
    'iso_build': 'ISO Build',
    'dev-plan/00_总览': '📋 Project Overview',
    'dev-plan/01_设计语言': '🎨 Design Language',
    'dev-plan/02_内核': '🔧 Kernel',
    'dev-plan/03_文件系统': '📁 Filesystem',
    'dev-plan/04_音频系统': '🔊 Audio System',
    'dev-plan/05_驱动层': '🔌 Drivers',
    'dev-plan/06_桌面环境': '🖥 Desktop',
    'dev-plan/07_应用生态': '📱 Apps',
    'dev-plan/08_安全特性': '🔒 Security',
    'dev-plan/09_模块系统': '📦 Module System',
    'dev-plan/10_ISO与网站': '🌐 ISO & Website',
  },
  ja: {
    'architecture': 'システムアーキテクチャ',
    'module_spec': 'モジュール仕様',
    'building': 'ビルドと実行',
    'api_syscall': 'システムコールAPI',
    'api_services': 'サービスAPI',
    'developer_guide': '開発者ガイド',
    'app_store': 'アプリストア仕様',
    'iso_build': 'ISOビルド',
    'dev-plan/00_总览': '📋 プロジェクト概要',
    'dev-plan/01_设计语言': '🎨 デザイン言語',
    'dev-plan/02_内核': '🔧 カーネル',
    'dev-plan/03_文件系统': '📁 ファイルシステム',
    'dev-plan/04_音频系统': '🔊 オーディオシステム',
    'dev-plan/05_驱动层': '🔌 ドライバ',
    'dev-plan/06_桌面环境': '🖥 デスクトップ',
    'dev-plan/07_应用生态': '📱 アプリ',
    'dev-plan/08_安全特性': '🔒 セキュリティ',
    'dev-plan/09_模块系统': '📦 モジュールシステム',
    'dev-plan/10_ISO与网站': '🌐 ISOとウェブサイト',
  },
  ko: {
    'architecture': '시스템 아키텍처',
    'module_spec': '모듈 사양',
    'building': '빌드 및 실행',
    'api_syscall': '시스템 콜 API',
    'api_services': '서비스 API',
    'developer_guide': '개발자 가이드',
    'app_store': '앱 스토어 사양',
    'iso_build': 'ISO 빌드',
    'dev-plan/00_总览': '📋 프로젝트 개요',
    'dev-plan/01_设计语言': '🎨 디자인 언어',
    'dev-plan/02_内核': '🔧 커널',
    'dev-plan/03_文件系统': '📁 파일시스템',
    'dev-plan/04_音频系统': '🔊 오디오 시스템',
    'dev-plan/05_驱动层': '🔌 드라이버',
    'dev-plan/06_桌面环境': '🖥 데스크톱',
    'dev-plan/07_应用生态': '📱 앱',
    'dev-plan/08_安全特性': '🔒 보안',
    'dev-plan/09_模块系统': '📦 모듈 시스템',
    'dev-plan/10_ISO与网站': '🌐 ISO 및 웹사이트',
  },
  ru: {
    'architecture': 'Архитектура системы',
    'module_spec': 'Спецификация модулей',
    'building': 'Сборка и запуск',
    'api_syscall': 'API системных вызовов',
    'api_services': 'API сервисов',
    'developer_guide': 'Руководство разработчика',
    'app_store': 'Спецификация магазина приложений',
    'iso_build': 'Сборка ISO',
    'dev-plan/00_总览': '📋 Обзор проекта',
    'dev-plan/01_设计语言': '🎨 Язык дизайна',
    'dev-plan/02_内核': '🔧 Ядро',
    'dev-plan/03_文件系统': '📁 Файловая система',
    'dev-plan/04_音频系统': '🔊 Аудио система',
    'dev-plan/05_驱动层': '🔌 Драйверы',
    'dev-plan/06_桌面环境': '🖥 Рабочий стол',
    'dev-plan/07_应用生态': '📱 Приложения',
    'dev-plan/08_安全特性': '🔒 Безопасность',
    'dev-plan/09_模块系统': '📦 Модульная система',
    'dev-plan/10_ISO与网站': '🌐 ISO и сайт',
  },
};

const FOOTER_I18N: Record<Lang, { docs: string; back: string; footer: string }> = {
  zh: { docs: '文档', back: '← 返回首页', footer: '© 2026 WebOS. MIT License.' },
  'zh-TW': { docs: '文檔', back: '← 返回首頁', footer: '© 2026 WebOS. MIT License.' },
  en: { docs: 'Docs', back: '← Back to Home', footer: '© 2026 WebOS. MIT License.' },
  ja: { docs: 'ドキュメント', back: '← ホームに戻る', footer: '© 2026 WebOS. MIT License.' },
  ko: { docs: '문서', back: '← 홈으로 돌아가기', footer: '© 2026 WebOS. MIT License.' },
  ru: { docs: 'Документация', back: '← На главную', footer: '© 2026 WebOS. MIT License.' },
};

const SIDEBAR_ITEMS = [
  { id: 'architecture' },
  { id: 'module_spec' },
  { id: 'building' },
  { id: 'api_syscall' },
  { id: 'api_services' },
  { id: 'developer_guide' },
  { id: 'app_store' },
  { id: 'iso_build' },
  { id: 'dev-plan/00_总览', section: true },
  { id: 'dev-plan/01_设计语言' },
  { id: 'dev-plan/02_内核' },
  { id: 'dev-plan/03_文件系统' },
  { id: 'dev-plan/04_音频系统' },
  { id: 'dev-plan/05_驱动层' },
  { id: 'dev-plan/06_桌面环境' },
  { id: 'dev-plan/07_应用生态' },
  { id: 'dev-plan/08_安全特性' },
  { id: 'dev-plan/09_模块系统' },
  { id: 'dev-plan/10_ISO与网站' },
];

// ============================================================================
// Icons
// ============================================================================
const CloseIcon = memo(() => (
  <svg viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" width="18" height="18">
    <path d="M18 6L6 18M6 6l12 12" />
  </svg>
));

// ============================================================================
// Layout
// ============================================================================
export default function DocsLayout({ children }: { children: React.ReactNode }) {
  const pathname = usePathname();
  const { lang, theme, mounted, setLang, toggleTheme } = useTheme();
  const [mobileOpen, setMobileOpen] = useState(false);
  const [isMobile, setIsMobile] = useState(false);

  const currentSlug = pathname?.split('/docs/')[1]?.replace(/\/$/, '') || 'architecture';
  const t = FOOTER_I18N[lang];

  useEffect(() => {
    const check = () => setIsMobile(window.innerWidth < 768);
    check();
    window.addEventListener('resize', check);
    return () => window.removeEventListener('resize', check);
  }, []);

  if (!mounted) {
    return (
      <div style={{ minHeight: '100vh', background: 'var(--bg)' }} suppressHydrationWarning>
        <Navbar
          active="docs"
          lang={lang}
          onLangChange={setLang}
          theme={theme}
          onThemeChange={toggleTheme}
        />
      </div>
    );
  }

  const sidebarContent = (
    <>
      <p className="docs-sidebar-title">{t.docs}</p>
      {SIDEBAR_ITEMS.map((item, index) => {
        if ((item as any).section) {
          return (
            <React.Fragment key={item.id}>
              {index > 0 && <div style={{ marginTop: 16, borderTop: '1px solid var(--border)', paddingTop: 12 }} />}
              <Link
                href={`/docs/${item.id}`}
                className={`sidebar-link ${currentSlug === item.id ? 'active' : ''}`}
                onClick={() => setMobileOpen(false)}
                style={{ fontWeight: 600, color: 'var(--text)' }}
              >
                {SIDEBAR_I18N[lang][item.id] || item.id}
              </Link>
            </React.Fragment>
          );
        }
        return (
          <Link
            key={item.id}
            href={`/docs/${item.id}`}
            className={`sidebar-link ${currentSlug === item.id ? 'active' : ''}`}
            onClick={() => setMobileOpen(false)}
          >
            {SIDEBAR_I18N[lang][item.id] || item.id}
          </Link>
        );
      })}
      <div style={{ marginTop: 20, borderTop: '1px solid var(--border)', paddingTop: 12 }}>
        <Link href="/intro" className="sidebar-link" style={{ color: 'var(--text-muted)' }}>
          {t.back}
        </Link>
      </div>
    </>
  );

  return (
    <div style={{ minHeight: '100vh', display: 'flex', flexDirection: 'column' }}>
      <Navbar
        active="docs"
        lang={lang}
        onLangChange={setLang}
        theme={theme}
        onThemeChange={toggleTheme}
        isMobile={isMobile}
        onMenuClick={isMobile ? () => setMobileOpen(true) : undefined}
      />

      {/* Mobile Drawer */}
      {mobileOpen && (
        <div className="mobile-drawer-overlay" onClick={() => setMobileOpen(false)}>
          <div className="mobile-drawer-panel" onClick={(e) => e.stopPropagation()}>
            <div className="mobile-drawer-header">
              <span>{t.docs}</span>
              <button className="icon-btn" onClick={() => setMobileOpen(false)} aria-label="关闭">
                <CloseIcon />
              </button>
            </div>
            {sidebarContent}
          </div>
        </div>
      )}

      {/* Body */}
      <div className="docs-body">
        {!isMobile && <aside className="docs-sidebar">{sidebarContent}</aside>}

        <div className="docs-main">
          <main className="docs-content-area">
            <div className="breadcrumb">
              <Link href="/docs">{t.docs}</Link>
              <span style={{ color: 'var(--text-muted)' }}>/</span>
              <span style={{ color: 'var(--text)' }}>
                {SIDEBAR_I18N[lang][currentSlug] || currentSlug}
              </span>
            </div>
            {children}
          </main>
          <footer className="page-footer">
            <p>{t.footer}</p>
          </footer>
        </div>
      </div>
    </div>
  );
}
