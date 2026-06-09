/**
 * WebOS Landing Page
 *
 * The main entry point for the website.
 * Provides access to the OS runtime and project information.
 */

import Link from 'next/link';

export default function Home() {
  return (
    <div className="landing">
      <div className="hero">
        <h1>WebOS</h1>
        <p className="subtitle">
          基于 C/WebAssembly 的浏览器操作系统 v0.0.b
        </p>
        <div className="actions">
          <Link href="/os" className="btn btn-primary">
            启动 WebOS
          </Link>
          <Link href="/docs" className="btn btn-secondary">
            开发文档
          </Link>
          <a
            href="https://github.com/DingdingOvO/webos"
            target="_blank"
            rel="noopener noreferrer"
            className="btn btn-secondary"
          >
            GitHub
          </a>
        </div>
      </div>

      <div className="features">
        <div className="feature-card">
          <h3>完全动态化</h3>
          <p>
            无静态链接，所有模块运行时通过 WASM 自定义段动态加载。
            支持 .wex 可执行文件、.Wdll 动态库、.wli 接口描述三种模块类型。
          </p>
        </div>
        <div className="feature-card">
          <h3>纯 C 内核</h3>
          <p>
            Bootloader、内核、驱动、系统服务全部使用 C 语言编写，
            编译为 WebAssembly 在浏览器中运行，提供完整的进程管理和 IPC。
          </p>
        </div>
        <div className="feature-card">
          <h3>C++ 应用</h3>
          <p>
            用户态应用使用 C++ 开发，通过系统调用和 IPC 与内核通信。
            支持窗口管理、文件系统、网络等完整的系统服务。
          </p>
        </div>
        <div className="feature-card">
          <h3>ISO 构建</h3>
          <p>
            支持构建为可启动的 ISO 内存操作系统镜像，
            基于 Linux + Chromium kiosk 模式，直接从 ISO 启动运行。
          </p>
        </div>
        <div className="feature-card">
          <h3>WebGPU 渲染</h3>
          <p>
            桌面环境使用 WebGPU 渲染，支持 Canvas 2D 回退。
            提供任务栏、开始菜单、窗口管理等完整的桌面体验。
          </p>
        </div>
        <div className="feature-card">
          <h3>完整测试</h3>
          <p>
            包含 C 单元测试、Python 工具测试、TypeScript 集成测试，
            完整的 CI 流水线确保每次提交的质量。
          </p>
        </div>
      </div>

      <footer style={{
        textAlign: 'center',
        padding: '2rem',
        color: 'var(--text-secondary)',
        borderTop: '1px solid var(--border)',
      }}>
        <p>WebOS v0.0.b — MIT License</p>
        <p style={{ marginTop: '0.5rem' }}>
          <a href="https://github.com/DingdingOvO/webos" style={{ color: 'var(--accent-light)' }}>
            GitHub
          </a>
          {' · '}
          <Link href="/docs" style={{ color: 'var(--accent-light)' }}>
            文档
          </Link>
        </p>
      </footer>
    </div>
  );
}
