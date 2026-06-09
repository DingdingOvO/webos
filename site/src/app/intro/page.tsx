'use client';

import { useTheme, Lang } from '../../components/useTheme';
import Navbar from '../../components/Navbar';

/* ============================================
   i18n translations for all 6 languages
   ============================================ */
const I18N: Record<Lang, {
  badge: string;
  title: string;
  subtitle: string;
  desc: string;
  primaryBtn: string;
  secondaryBtn: string;
  featuresTitle: string;
  featuresDesc: string;
  features: { icon: string; title: string; desc: string }[];
  techTitle: string;
  footer: string;
}> = {
  zh: {
    badge: '开源项目',
    title: 'WebOS',
    subtitle: '浏览器中的操作系统',
    desc: '基于 C/WebAssembly 架构，在浏览器中运行完整的操作系统。纯 C 内核、动态链接器、WebGPU 渲染，带来原生的系统体验。',
    primaryBtn: '立即体验',
    secondaryBtn: '查看文档',
    featuresTitle: '核心特性',
    featuresDesc: '为浏览器而生的操作系统',
    features: [
      { icon: 'monitor', title: '桌面环境', desc: '完整的窗口管理、任务栏与开始菜单，通过 WebGPU 加速渲染，提供流畅的桌面体验。' },
      { icon: 'folder', title: '文件系统', desc: '虚拟文件系统，基于浏览器本地存储持久化，支持目录结构与文件操作。' },
      { icon: 'globe', title: '多语言支持', desc: '内置中英文界面，支持简体中文、繁体中文、英语、日语、韩语、俄语。' },
      { icon: 'shield', title: '安全加密', desc: '本地数据加密存储，保护用户隐私，所有数据仅在浏览器本地处理。' },
      { icon: 'cpu', title: '动态加载', desc: '运行时通过 WASM 自定义段动态加载 .wex/.Wdll/.wli 模块，无静态链接。' },
      { icon: 'disc', title: 'ISO 构建', desc: '支持构建为可启动的 ISO 内存操作系统镜像，基于 Linux + Chromium kiosk 模式。' },
    ],
    techTitle: '技术栈',
    footer: '© 2026 WebOS. MIT License.',
  },
  'zh-TW': {
    badge: '開源專案',
    title: 'WebOS',
    subtitle: '瀏覽器中的作業系統',
    desc: '基於 C/WebAssembly 架構，在瀏覽器中運行完整的作業系統。純 C 內核、動態連結器、WebGPU 渲染，帶來原生的系統體驗。',
    primaryBtn: '立即體驗',
    secondaryBtn: '查看文檔',
    featuresTitle: '核心特性',
    featuresDesc: '為瀏覽器而生的作業系統',
    features: [
      { icon: 'monitor', title: '桌面環境', desc: '完整的視窗管理、任務欄與開始選單，透過 WebGPU 加速渲染，提供流暢的桌面體驗。' },
      { icon: 'folder', title: '檔案系統', desc: '虛擬檔案系統，基於瀏覽器本地儲存持久化，支援目錄結構與檔案操作。' },
      { icon: 'globe', title: '多語言支援', desc: '內建中英文介面，支援簡體中文、繁體中文、英語、日語、韓語、俄語。' },
      { icon: 'shield', title: '安全加密', desc: '本地資料加密儲存，保護使用者隱私，所有資料僅在瀏覽器本地處理。' },
      { icon: 'cpu', title: '動態載入', desc: '運行時透過 WASM 自定義段動態載入 .wex/.Wdll/.wli 模組，無靜態連結。' },
      { icon: 'disc', title: 'ISO 建置', desc: '支援建置為可啟動的 ISO 記憶體作業系統映像，基於 Linux + Chromium kiosk 模式。' },
    ],
    techTitle: '技術棧',
    footer: '© 2026 WebOS. MIT License.',
  },
  en: {
    badge: 'Open Source',
    title: 'WebOS',
    subtitle: 'Operating System in Your Browser',
    desc: 'Built on C/WebAssembly architecture, running a complete operating system in the browser. Pure C kernel, dynamic linker, WebGPU rendering for a native system experience.',
    primaryBtn: 'Try Now',
    secondaryBtn: 'Read Docs',
    featuresTitle: 'Core Features',
    featuresDesc: 'An operating system designed for the browser',
    features: [
      { icon: 'monitor', title: 'Desktop Environment', desc: 'Full window management, taskbar and start menu with WebGPU accelerated rendering for a smooth desktop experience.' },
      { icon: 'folder', title: 'File System', desc: 'Virtual file system persisted via browser local storage, supporting directory structures and file operations.' },
      { icon: 'globe', title: 'i18n Support', desc: 'Built-in multilingual interface supporting Simplified Chinese, Traditional Chinese, English, Japanese, Korean, and Russian.' },
      { icon: 'shield', title: 'Security', desc: 'Encrypted local data storage protecting user privacy. All data is processed locally in the browser only.' },
      { icon: 'cpu', title: 'Dynamic Loading', desc: 'Runtime dynamic loading of .wex/.Wdll/.wli modules via WASM custom sections, no static linking.' },
      { icon: 'disc', title: 'ISO Build', desc: 'Build bootable ISO memory OS images based on Linux + Chromium kiosk mode for bare-metal deployment.' },
    ],
    techTitle: 'Tech Stack',
    footer: '© 2026 WebOS. MIT License.',
  },
  ja: {
    badge: 'オープンソース',
    title: 'WebOS',
    subtitle: 'ブラウザ上のオペレーティングシステム',
    desc: 'C/WebAssemblyアーキテクチャに基づき、ブラウザで完全なオペレーティングシステムを実行。純粋なCカーネル、動的リンカー、WebGPUレンダリングによるネイティブなシステム体験。',
    primaryBtn: '今すぐ試す',
    secondaryBtn: 'ドキュメント',
    featuresTitle: 'コア機能',
    featuresDesc: 'ブラウザのために設計されたOS',
    features: [
      { icon: 'monitor', title: 'デスクトップ環境', desc: 'WebGPUアクセラレーションレンダリングによる、完全なウィンドウ管理、タスクバー、スタートメニュー。' },
      { icon: 'folder', title: 'ファイルシステム', desc: 'ブラウザローカルストレージによる仮想ファイルシステム、ディレクトリ構造とファイル操作をサポート。' },
      { icon: 'globe', title: '多言語対応', desc: '簡体字中国語、繁体字中国語、英語、日本語、韓国語、ロシア語をサポートする多言語インターフェース。' },
      { icon: 'shield', title: 'セキュリティ', desc: 'ローカルデータの暗号化保存でプライバシーを保護。すべてのデータはブラウザ内でのみ処理。' },
      { icon: 'cpu', title: '動的ローディング', desc: 'WASMカスタムセクションを介した.wex/.Wdll/.wliモジュールのランタイム動的ローディング。' },
      { icon: 'disc', title: 'ISOビルド', desc: 'Linux + Chromiumキオスクモードベースの起動可能なISOメモリOSイメージの構築をサポート。' },
    ],
    techTitle: 'テクノロジースタック',
    footer: '© 2026 WebOS. MIT License.',
  },
  ko: {
    badge: '오픈 소스',
    title: 'WebOS',
    subtitle: '브라우저 속 운영 체제',
    desc: 'C/WebAssembly 아키텍처 기반으로 브라우저에서 완전한 운영 체제를 실행합니다. 순수 C 커널, 동적 링커, WebGPU 렌더링으로 네이티브 시스템 경험을 제공합니다.',
    primaryBtn: '지금 체험',
    secondaryBtn: '문서 보기',
    featuresTitle: '핵심 기능',
    featuresDesc: '브라우저를 위해 설계된 OS',
    features: [
      { icon: 'monitor', title: '데스크톱 환경', desc: 'WebGPU 가속 렌더링으로 완전한 창 관리, 작업 표시줄 및 시작 메뉴를 제공합니다.' },
      { icon: 'folder', title: '파일 시스템', desc: '브라우저 로컬 스토리지 기반 가상 파일 시스템, 디렉토리 구조 및 파일 작업 지원.' },
      { icon: 'globe', title: '다국어 지원', desc: '간체 중국어, 번체 중국어, 영어, 일본어, 한국어, 러시아어를 지원하는 다국어 인터페이스.' },
      { icon: 'shield', title: '보안 암호화', desc: '로컬 데이터 암호화 저장으로 개인정보 보호. 모든 데이터는 브라우저 로컬에서만 처리됩니다.' },
      { icon: 'cpu', title: '동적 로딩', desc: 'WASM 커스텀 섹션을 통한 .wex/.Wdll/.wli 모듈의 런타임 동적 로딩, 정적 링크 없음.' },
      { icon: 'disc', title: 'ISO 빌드', desc: 'Linux + Chromium 키오스크 모드 기반 부팅 가능한 ISO 메모리 OS 이미지 구축 지원.' },
    ],
    techTitle: '기술 스택',
    footer: '© 2026 WebOS. MIT License.',
  },
  ru: {
    badge: 'Открытый исходный код',
    title: 'WebOS',
    subtitle: 'Операционная система в браузере',
    desc: 'Построена на архитектуре C/WebAssembly, работает как полноценная ОС в браузере. Чистое ядро на C, динамический компоновщик, рендеринг WebGPU для нативного системного опыта.',
    primaryBtn: 'Попробовать',
    secondaryBtn: 'Документация',
    featuresTitle: 'Ключевые возможности',
    featuresDesc: 'ОС, созданная для браузера',
    features: [
      { icon: 'monitor', title: 'Рабочий стол', desc: 'Полное управление окнами, панель задач и меню пуск с ускоренным рендерингом WebGPU.' },
      { icon: 'folder', title: 'Файловая система', desc: 'Виртуальная файловая система на основе локального хранилища браузера с поддержкой каталогов и файловых операций.' },
      { icon: 'globe', title: 'Многоязычность', desc: 'Встроенный многоязычный интерфейс: упрощённый и традиционный китайский, английский, японский, корейский, русский.' },
      { icon: 'shield', title: 'Безопасность', desc: 'Шифрование локальных данных для защиты конфиденциальности. Все данные обрабатываются только в браузере.' },
      { icon: 'cpu', title: 'Динамическая загрузка', desc: 'Динамическая загрузка модулей .wex/.Wdll/.wli через пользовательские секции WASM без статической линковки.' },
      { icon: 'disc', title: 'ISO-сборка', desc: 'Поддержка сборки загрузочных ISO-образов ОС на основе Linux + Chromium в режиме киоска.' },
    ],
    techTitle: 'Технологический стек',
    footer: '© 2026 WebOS. MIT License.',
  },
};

/* ============================================
   SVG Icons
   ============================================ */
const ICONS: Record<string, JSX.Element> = {
  monitor: (
    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <rect x="2" y="3" width="20" height="14" rx="2" ry="2" />
      <line x1="8" y1="21" x2="16" y2="21" />
      <line x1="12" y1="17" x2="12" y2="21" />
    </svg>
  ),
  folder: (
    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <path d="M22 19a2 2 0 0 1-2 2H4a2 2 0 0 1-2-2V5a2 2 0 0 1 2-2h5l2 3h9a2 2 0 0 1 2 2z" />
    </svg>
  ),
  globe: (
    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <circle cx="12" cy="12" r="10" />
      <line x1="2" y1="12" x2="22" y2="12" />
      <path d="M12 2a15.3 15.3 0 0 1 4 10 15.3 15.3 0 0 1-4 10 15.3 15.3 0 0 1-4-10 15.3 15.3 0 0 1 4-10z" />
    </svg>
  ),
  shield: (
    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <path d="M12 22s8-4 8-10V5l-8-3-8 3v7c0 6 8 10 8 10z" />
    </svg>
  ),
  cpu: (
    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <rect x="4" y="4" width="16" height="16" rx="2" ry="2" />
      <rect x="9" y="9" width="6" height="6" />
      <line x1="9" y1="1" x2="9" y2="4" />
      <line x1="15" y1="1" x2="15" y2="4" />
      <line x1="9" y1="20" x2="9" y2="23" />
      <line x1="15" y1="20" x2="15" y2="23" />
      <line x1="20" y1="9" x2="23" y2="9" />
      <line x1="20" y1="14" x2="23" y2="14" />
      <line x1="1" y1="9" x2="4" y2="9" />
      <line x1="1" y1="14" x2="4" y2="14" />
    </svg>
  ),
  disc: (
    <svg width="22" height="22" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
      <circle cx="12" cy="12" r="10" />
      <circle cx="12" cy="12" r="3" />
      <line x1="12" y1="2" x2="12" y2="5" />
    </svg>
  ),
};

/* ============================================
   Intro Page Component
   ============================================ */
export default function IntroPage() {
  const { lang, setLang, theme, toggleTheme, mounted } = useTheme();
  const t = I18N[lang];

  return (
    <div style={{ minHeight: '100vh', display: 'flex', flexDirection: 'column' }}>
      <Navbar lang={lang} setLang={setLang} theme={theme} toggleTheme={toggleTheme} mounted={mounted} />

      <main style={{ marginTop: 'var(--navbar-height)', flex: 1 }}>
        {/* Hero Section */}
        <section className="hero">
          <div className="hero-badge">
            <span className="hero-badge-dot" />
            {t.badge}
          </div>
          <h1 className="hero-title">{t.title}</h1>
          <p className="hero-subtitle">{t.subtitle}</p>
          <p className="hero-desc">{t.desc}</p>
          <div className="hero-actions">
            <a href="/app/os" className="btn btn-primary">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <polygon points="5 3 19 12 5 21 5 3" />
              </svg>
              {t.primaryBtn}
            </a>
            <a href="/docs" className="btn btn-secondary">
              <svg width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" strokeWidth="2" strokeLinecap="round" strokeLinejoin="round">
                <path d="M2 3h6a4 4 0 0 1 4 4v14a3 3 0 0 0-3-3H2z" />
                <path d="M22 3h-6a4 4 0 0 0-4 4v14a3 3 0 0 1 3-3h7z" />
              </svg>
              {t.secondaryBtn}
            </a>
          </div>
        </section>

        {/* Features Section */}
        <section className="features-section">
          <div className="features-header">
            <h2>{t.featuresTitle}</h2>
            <p>{t.featuresDesc}</p>
          </div>
          <div className="features-grid">
            {t.features.map((feature, i) => (
              <div key={i} className="feature-card">
                <div className="feature-icon">
                  {ICONS[feature.icon]}
                </div>
                <h3>{feature.title}</h3>
                <p>{feature.desc}</p>
              </div>
            ))}
          </div>
        </section>

        {/* Tech Stack Section */}
        <section className="tech-section">
          <h2>{t.techTitle}</h2>
          <div className="tech-grid">
            {['C', 'C++', 'WebAssembly', 'TypeScript', 'WebGPU'].map((tech) => (
              <span key={tech} className="tech-item">{tech}</span>
            ))}
          </div>
        </section>
      </main>

      {/* Footer */}
      <footer className="site-footer">
        <p>{t.footer}</p>
        <p style={{ marginTop: '0.5rem' }}>
          <a href="https://github.com/DingdingOvO/webos" target="_blank" rel="noopener noreferrer">GitHub</a>
          {' · '}
          <a href="/docs">Docs</a>
        </p>
      </footer>
    </div>
  );
}
