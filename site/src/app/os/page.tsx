/**
 * WebOS Runtime Page
 *
 * Loads and runs the WebOS in the browser.
 * This is the "app" route that embeds the OS.
 */

'use client';

import { useEffect, useState } from 'react';

type BootState = 'loading' | 'ready' | 'running' | 'error';

export default function OSPage() {
  const [bootState, setBootState] = useState<BootState>('loading');
  const [errorMsg, setErrorMsg] = useState('');

  useEffect(() => {
    // Try to load the WebOS host
    const initOS = async () => {
      try {
        // Check WebAssembly support
        if (typeof WebAssembly === 'undefined') {
          throw new Error('WebAssembly is not supported in this browser');
        }

        // Check WebGPU support (optional)
        const hasWebGPU = !!navigator.gpu;
        console.log(`[WebOS] WebAssembly: supported, WebGPU: ${hasWebGPU ? 'available' : 'not available'}`);

        setBootState('ready');
      } catch (err: any) {
        setBootState('error');
        setErrorMsg(err.message);
      }
    };

    initOS();
  }, []);

  const startOS = () => {
    setBootState('running');
  };

  if (bootState === 'error') {
    return (
      <div style={{
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        height: '100vh', background: '#0a0a1a', color: '#ff4444',
        fontFamily: 'monospace', padding: '2rem', textAlign: 'center',
      }}>
        <div>
          <h2>WebOS 启动失败</h2>
          <p style={{ margin: '1rem 0', color: '#aaa' }}>{errorMsg}</p>
          <p>请使用支持 WebAssembly 的现代浏览器（Chrome 88+、Firefox 89+、Safari 15+）</p>
        </div>
      </div>
    );
  }

  if (bootState === 'loading') {
    return (
      <div style={{
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        height: '100vh', background: '#0a0a1a', color: '#ccc',
        fontFamily: '"Segoe UI", sans-serif',
      }}>
        <div style={{ textAlign: 'center' }}>
          <div style={{
            width: 40, height: 40, margin: '0 auto 1rem',
            border: '3px solid #333', borderTopColor: '#0078d4',
            borderRadius: '50%', animation: 'spin 1s linear infinite',
          }} />
          <p>正在检测运行环境...</p>
          <style>{`@keyframes spin { to { transform: rotate(360deg); } }`}</style>
        </div>
      </div>
    );
  }

  if (bootState === 'ready') {
    return (
      <div style={{
        display: 'flex', alignItems: 'center', justifyContent: 'center',
        height: '100vh', background: '#0a0a1a', color: '#ccc',
        fontFamily: '"Segoe UI", sans-serif',
      }}>
        <div style={{ textAlign: 'center' }}>
          <h1 style={{ fontSize: '3rem', marginBottom: '1rem', color: '#0078d4' }}>WebOS</h1>
          <p style={{ marginBottom: '2rem', color: '#888' }}>v0.0.b — 运行环境就绪</p>
          <button
            onClick={startOS}
            style={{
              padding: '0.75rem 2.5rem', fontSize: '1.1rem',
              background: '#0078d4', color: 'white', border: 'none',
              borderRadius: 8, cursor: 'pointer', fontWeight: 600,
            }}
          >
            启动系统
          </button>
          <p style={{ marginTop: '1rem', fontSize: '0.85rem', color: '#666' }}>
            点击上方按钮启动 WebOS（需要 WASM 模块已构建）
          </p>
        </div>
      </div>
    );
  }

  // Running state - show the OS canvas
  return (
    <div className="os-container">
      <canvas id="webos-canvas"></canvas>
      <div id="boot-status" style={{ display: 'none' }}>
        <div className="spinner"></div>
        <p>WebOS v0.0.b — 正在启动...</p>
      </div>
      {/* The TypeScript host (main.ts) will attach to this canvas */}
    </div>
  );
}
