'use client';

import { useState, useEffect } from 'react';
import { usePathname } from 'next/navigation';
import { useTheme, Lang } from '../../components/useTheme';
import Navbar from '../../components/Navbar';

const DOCS_SIDEBAR_I18N: Record<Lang, { title: string; items: { slug: string; label: string }[]; mobileOpen: string; back: string }> = {
  zh: {
    title: '文档',
    items: [
      { slug: 'architecture', label: '系统架构' },
      { slug: 'module_spec', label: '模块格式规范' },
      { slug: 'building', label: '构建与运行' },
      { slug: 'api_syscall', label: '系统调用 API' },
      { slug: 'api_services', label: '系统服务 API' },
      { slug: 'developer_guide', label: '应用开发指南' },
      { slug: 'app_store', label: '应用商店规范' },
    ],
    mobileOpen: '打开目录',
    back: '← 返回文档',
  },
  'zh-TW': {
    title: '文檔',
    items: [
      { slug: 'architecture', label: '系統架構' },
      { slug: 'module_spec', label: '模組格式規範' },
      { slug: 'building', label: '建置與執行' },
      { slug: 'api_syscall', label: '系統呼叫 API' },
      { slug: 'api_services', label: '系統服務 API' },
      { slug: 'developer_guide', label: '應用開發指南' },
      { slug: 'app_store', label: '應用商店規範' },
    ],
    mobileOpen: '開啟目錄',
    back: '← 返回文檔',
  },
  en: {
    title: 'Documentation',
    items: [
      { slug: 'architecture', label: 'System Architecture' },
      { slug: 'module_spec', label: 'Module Specification' },
      { slug: 'building', label: 'Building & Running' },
      { slug: 'api_syscall', label: 'Syscall API' },
      { slug: 'api_services', label: 'Services API' },
      { slug: 'developer_guide', label: 'Developer Guide' },
      { slug: 'app_store', label: 'App Store Specification' },
    ],
    mobileOpen: 'Open Menu',
    back: '← Back to Docs',
  },
  ja: {
    title: 'ドキュメント',
    items: [
      { slug: 'architecture', label: 'システムアーキテクチャ' },
      { slug: 'module_spec', label: 'モジュール仕様' },
      { slug: 'building', label: 'ビルドと実行' },
      { slug: 'api_syscall', label: 'システムコールAPI' },
      { slug: 'api_services', label: 'サービスAPI' },
      { slug: 'developer_guide', label: '開発者ガイド' },
      { slug: 'app_store', label: 'アプリストア仕様' },
    ],
    mobileOpen: 'メニューを開く',
    back: '← ドキュメントに戻る',
  },
  ko: {
    title: '문서',
    items: [
      { slug: 'architecture', label: '시스템 아키텍처' },
      { slug: 'module_spec', label: '모듈 사양' },
      { slug: 'building', label: '빌드 및 실행' },
      { slug: 'api_syscall', label: '시스템 콜 API' },
      { slug: 'api_services', label: '서비스 API' },
      { slug: 'developer_guide', label: '개발자 가이드' },
      { slug: 'app_store', label: '앱 스토어 사양' },
    ],
    mobileOpen: '메뉴 열기',
    back: '← 문서로 돌아가기',
  },
  ru: {
    title: 'Документация',
    items: [
      { slug: 'architecture', label: 'Архитектура системы' },
      { slug: 'module_spec', label: 'Спецификация модулей' },
      { slug: 'building', label: 'Сборка и запуск' },
      { slug: 'api_syscall', label: 'API системных вызовов' },
      { slug: 'api_services', label: 'API сервисов' },
      { slug: 'developer_guide', label: 'Руководство разработчика' },
      { slug: 'app_store', label: 'Спецификация магазина приложений' },
    ],
    mobileOpen: 'Открыть меню',
    back: '← Назад к документации',
  },
};

export default function DocsLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  const pathname = usePathname();
  const { lang, setLang, theme, toggleTheme, mounted } = useTheme();
  const [drawerOpen, setDrawerOpen] = useState(false);

  const t = DOCS_SIDEBAR_I18N[lang];
  const currentSlug = pathname?.split('/').filter(Boolean).pop() || '';

  // Close drawer on route change
  useEffect(() => {
    setDrawerOpen(false);
  }, [pathname]);

  return (
    <div style={{ minHeight: '100vh', display: 'flex', flexDirection: 'column' }}>
      <Navbar lang={lang} setLang={setLang} theme={theme} toggleTheme={toggleTheme} mounted={mounted} />

      <div className="docs-layout">
        {/* Desktop Sidebar */}
        <aside className="docs-sidebar">
          <div className="docs-sidebar-title">{t.title}</div>
          {t.items.map((item) => (
            <a
              key={item.slug}
              href={`/docs/${item.slug}`}
              className={currentSlug === item.slug ? 'active' : ''}
            >
              {item.label}
            </a>
          ))}
        </aside>

        {/* Content Area */}
        <div className="docs-content">
          <div className="docs-breadcrumb">
            <a href="/docs">{t.title}</a>
            {currentSlug && (
              <>
                {' / '}
                <span>{t.items.find((i) => i.slug === currentSlug)?.label || currentSlug}</span>
              </>
            )}
          </div>
          {children}
        </div>
      </div>

      {/* Mobile drawer button */}
      <button
        className="docs-drawer-btn"
        onClick={() => setDrawerOpen(!drawerOpen)}
        aria-label={t.mobileOpen}
      >
        <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
          <line x1="3" y1="12" x2="21" y2="12" />
          <line x1="3" y1="6" x2="21" y2="6" />
          <line x1="3" y1="18" x2="21" y2="18" />
        </svg>
      </button>

      {/* Mobile drawer overlay */}
      {drawerOpen && (
        <div className="docs-mobile-drawer open" onClick={() => setDrawerOpen(false)}>
          <div className="docs-mobile-drawer-content" onClick={(e) => e.stopPropagation()}>
            <div className="docs-sidebar-title">{t.title}</div>
            {t.items.map((item) => (
              <a
                key={item.slug}
                href={`/docs/${item.slug}`}
                className={currentSlug === item.slug ? 'active' : ''}
                onClick={() => setDrawerOpen(false)}
              >
                {item.label}
              </a>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}
