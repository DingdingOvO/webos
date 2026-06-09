'use client';

import { useState, useEffect, useCallback } from 'react';

/* ============================================
   OS State Types
   ============================================ */
export type OSStage = 'boot' | 'ready' | 'running' | 'error';

export interface OSState {
  stage: OSStage;
  progress: number;
  errorMsg: string;
  hasWasm: boolean;
  hasWebGPU: boolean;
  bootMessages: string[];
}

/* ============================================
   Boot Animation Component
   ============================================ */
function BootAnimation({ progress, messages }: { progress: number; messages: string[] }) {
  return (
    <div className="boot-screen">
      <div className="boot-logo">WebOS</div>
      <div className="boot-version">v0.0.1beta</div>
      <div className="boot-spinner" />
      <div className="boot-status">
        {messages.length > 0 && messages[messages.length - 1]}
      </div>
      <div className="boot-progress">
        <div className="boot-progress-bar" style={{ width: `${progress}%` }} />
      </div>
    </div>
  );
}

/* ============================================
   Desktop Stage Component
   ============================================ */
function DesktopStage() {
  return (
    <div className="desktop-stage">
      <canvas id="webos-canvas" />
      <div className="desktop-taskbar">
        <button className="desktop-start-btn">WebOS</button>
        <div style={{ flex: 1 }} />
        <span style={{ color: '#71717a', fontSize: '0.75rem', fontFamily: 'var(--font-mono)' }}>
          v0.0.1beta
        </span>
      </div>
    </div>
  );
}

/* ============================================
   Main OS Component
   ============================================ */
export default function OSPage() {
  const [state, setState] = useState<OSState>({
    stage: 'boot',
    progress: 0,
    errorMsg: '',
    hasWasm: false,
    hasWebGPU: false,
    bootMessages: [],
  });

  const addMessage = useCallback((msg: string) => {
    setState((prev) => ({
      ...prev,
      bootMessages: [...prev.bootMessages, msg],
    }));
  }, []);

  useEffect(() => {
    let cancelled = false;

    const boot = async () => {
      // Check WebAssembly
      const hasWasm = typeof WebAssembly !== 'undefined';
      if (!hasWasm) {
        setState((prev) => ({
          ...prev,
          stage: 'error',
          errorMsg: 'WebAssembly is not supported in this browser',
        }));
        return;
      }

      if (cancelled) return;
      addMessage('WebAssembly: supported ✓');
      setState((prev) => ({ ...prev, hasWasm: true, progress: 20 }));

      // Check WebGPU
      const hasWebGPU = !!navigator.gpu;
      await new Promise((r) => setTimeout(r, 400));
      if (cancelled) return;
      addMessage(`WebGPU: ${hasWebGPU ? 'available ✓' : 'not available (Canvas 2D fallback)'}`);
      setState((prev) => ({ ...prev, hasWebGPU, progress: 40 }));

      // Simulate kernel loading
      await new Promise((r) => setTimeout(r, 500));
      if (cancelled) return;
      addMessage('Loading kernel.wasm...');
      setState((prev) => ({ ...prev, progress: 60 }));

      // Simulate bootloader init
      await new Promise((r) => setTimeout(r, 400));
      if (cancelled) return;
      addMessage('Initializing bootloader...');
      setState((prev) => ({ ...prev, progress: 75 }));

      // Simulate services
      await new Promise((r) => setTimeout(r, 400));
      if (cancelled) return;
      addMessage('Starting system services...');
      setState((prev) => ({ ...prev, progress: 90 }));

      // Ready
      await new Promise((r) => setTimeout(r, 300));
      if (cancelled) return;
      addMessage('Boot complete. System ready.');
      setState((prev) => ({ ...prev, progress: 100, stage: 'ready' }));
    };

    boot();

    return () => {
      cancelled = true;
    };
  }, [addMessage]);

  const handleLaunch = () => {
    setState((prev) => ({ ...prev, stage: 'running' }));
  };

  // Error state
  if (state.stage === 'error') {
    return (
      <div className="boot-screen">
        <div className="boot-error">
          <h2>⚠ WebOS 启动失败</h2>
          <p>{state.errorMsg}</p>
          <p>请使用支持 WebAssembly 的现代浏览器（Chrome 88+、Firefox 89+、Safari 15+）</p>
          <br />
          <a href="/intro" className="btn btn-secondary" style={{ fontSize: '0.85rem' }}>
            ← 返回首页
          </a>
        </div>
      </div>
    );
  }

  // Boot animation
  if (state.stage === 'boot') {
    return <BootAnimation progress={state.progress} messages={state.bootMessages} />;
  }

  // Ready state
  if (state.stage === 'ready') {
    return (
      <div className="boot-screen">
        <div className="boot-ready">
          <h1>WebOS</h1>
          <p>v0.0.1beta — 运行环境就绪</p>
          <p style={{ fontSize: '0.8rem', color: '#52525b', marginBottom: '0.5rem' }}>
            WebAssembly: ✓ &nbsp;|&nbsp; WebGPU: {state.hasWebGPU ? '✓' : '✗ (Canvas 2D)'}
          </p>
          <button className="btn btn-primary" onClick={handleLaunch}>
            启动系统
          </button>
          <p style={{ marginTop: '1rem', fontSize: '0.8rem', color: '#52525b' }}>
            点击上方按钮启动 WebOS
          </p>
        </div>
      </div>
    );
  }

  // Running state - show desktop
  return <DesktopStage />;
}
