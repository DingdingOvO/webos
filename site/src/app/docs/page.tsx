/**
 * Documentation Index Page
 */

import Link from 'next/link';

interface DocInfo {
  slug: string;
  title: string;
  description: string;
}

const docs: DocInfo[] = [
  { slug: 'architecture', title: '系统架构', description: '整体分层、模块加载流程、动态链接器原理' },
  { slug: 'module_spec', title: '模块格式规范', description: '后缀名定义、自定义段格式、.wli JSON 规范' },
  { slug: 'building', title: '构建与运行', description: '环境准备、构建命令、常见问题' },
  { slug: 'api_syscall', title: '系统调用 API', description: '编号范围、参数说明、使用示例' },
  { slug: 'api_services', title: '系统服务 API', description: '窗口管理器、文件系统、网络服务接口' },
  { slug: 'developer_guide', title: '应用开发指南', description: 'C++ 应用开发完整教程' },
  { slug: 'app_store', title: '应用商店规范', description: 'app.json 格式、提交流程、安装机制' },
];

export default function DocsPage() {
  return (
    <div style={{
      maxWidth: 900, margin: '0 auto', padding: '3rem 2rem',
      fontFamily: '"Segoe UI", sans-serif',
    }}>
      <h1 style={{ fontSize: '2.5rem', marginBottom: '0.5rem', color: '#0078d4' }}>
        WebOS 文档
      </h1>
      <p style={{ color: '#888', marginBottom: '2rem' }}>v0.0.b</p>

      <div style={{ display: 'grid', gap: '1rem' }}>
        {docs.map((doc) => (
          <Link
            key={doc.slug}
            href={`/docs/${doc.slug}`}
            style={{
              display: 'block', padding: '1.5rem',
              background: '#16213e', borderRadius: 12,
              border: '1px solid #2a2a4a',
              textDecoration: 'none', color: 'inherit',
              transition: 'transform 0.2s',
            }}
          >
            <h3 style={{ color: '#2196f3', marginBottom: '0.25rem' }}>{doc.title}</h3>
            <p style={{ color: '#a0a0a0', fontSize: '0.9rem' }}>{doc.description}</p>
          </Link>
        ))}
      </div>
    </div>
  );
}
