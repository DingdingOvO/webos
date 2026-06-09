/**
 * Dynamic Documentation Page
 *
 * Reads markdown from the docs directory and renders it.
 */

import fs from 'fs';
import path from 'path';

interface PageProps {
  params: Promise<{ slug: string }>;
}

// Simple markdown to HTML converter (basic)
function markdownToHtml(md: string): string {
  let html = md
    // Headers
    .replace(/^### (.+)$/gm, '<h3>$1</h3>')
    .replace(/^## (.+)$/gm, '<h2>$1</h2>')
    .replace(/^# (.+)$/gm, '<h1>$1</h1>')
    // Code blocks
    .replace(/```(\w*)\n([\s\S]*?)```/g, '<pre><code class="language-$1">$2</code></pre>')
    // Inline code
    .replace(/`([^`]+)`/g, '<code>$1</code>')
    // Bold
    .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
    // Italic
    .replace(/\*([^*]+)\*/g, '<em>$1</em>')
    // Links
    .replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>')
    // Tables (basic)
    .replace(/\|(.+)\|/g, (match) => {
      const cells = match.split('|').filter(c => c.trim());
      if (cells.every(c => /^[\s-:]+$/.test(c))) return '';
      return '<tr>' + cells.map(c => `<td>${c.trim()}</td>`).join('') + '</tr>';
    })
    // Unordered lists
    .replace(/^- (.+)$/gm, '<li>$1</li>')
    // Paragraphs (lines not already wrapped)
    .replace(/^(?!<[huptdol]|<li|<tr|<pre|<code)(.+)$/gm, '<p>$1</p>')
    // Line breaks
    .replace(/\n\n/g, '\n');

  return html;
}

const docsDir = path.join(process.cwd(), '..', '..', 'docs');

export async function generateStaticParams() {
  const files = fs.readdirSync(docsDir).filter(f => f.endsWith('.md'));
  return files.map(f => ({ slug: f.replace('.md', '') }));
}

export default async function DocPage({ params }: PageProps) {
  const { slug } = await params;
  const filePath = path.join(docsDir, `${slug}.md`);

  let content = '文档未找到';
  try {
    content = fs.readFileSync(filePath, 'utf-8');
  } catch {
    content = `# 文档未找到: ${slug}`;
  }

  const html = markdownToHtml(content);

  return (
    <div style={{
      maxWidth: 900, margin: '0 auto', padding: '3rem 2rem',
      fontFamily: '"Segoe UI", sans-serif',
    }}>
      <a href="/docs" style={{ color: '#2196f3', fontSize: '0.9rem' }}>
        ← 返回文档列表
      </a>
      <div
        style={{ marginTop: '1rem' }}
        dangerouslySetInnerHTML={{ __html: html }}
      />
    </div>
  );
}
