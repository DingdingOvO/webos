import type { Metadata } from 'next';
import './globals.css';

export const metadata: Metadata = {
  title: 'WebOS v0.0.b',
  description: '基于 C/WebAssembly 的浏览器操作系统',
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="zh-CN">
      <body>{children}</body>
    </html>
  );
}
