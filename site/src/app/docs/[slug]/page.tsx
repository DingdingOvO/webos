/**
 * Dynamic Documentation Page
 *
 * Reads markdown from the docs directory and renders it.
 * Uses a basic markdown-to-HTML converter for static export compatibility.
 */

import fs from 'fs';
import path from 'path';

interface PageProps {
  params: Promise<{ slug: string }>;
}

// Simple markdown to HTML converter
function markdownToHtml(md: string): string {
  let html = md
    // Escape HTML entities in code blocks first (we'll handle this in code block processing)
    // Headers
    .replace(/^#### (.+)$/gm, '<h4>$1</h4>')
    .replace(/^### (.+)$/gm, '<h3>$1</h3>')
    .replace(/^## (.+)$/gm, '<h2>$1</h2>')
    .replace(/^# (.+)$/gm, '<h1>$1</h1>')
    // Code blocks with language
    .replace(/```(\w*)\n([\s\S]*?)```/g, (_match, lang, code) => {
      const escaped = code
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
      return `<pre><code class="language-${lang}">${escaped}</code></pre>`;
    })
    // Inline code
    .replace(/`([^`]+)`/g, (_match, code) => {
      const escaped = code
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;');
      return `<code>${escaped}</code>`;
    })
    // Bold
    .replace(/\*\*([^*]+)\*\*/g, '<strong>$1</strong>')
    // Italic
    .replace(/\*([^*]+)\*/g, '<em>$1</em>')
    // Links
    .replace(/\[([^\]]+)\]\(([^)]+)\)/g, '<a href="$2">$1</a>')
    // Horizontal rule
    .replace(/^---$/gm, '<hr />')
    // Unordered lists
    .replace(/^- (.+)$/gm, '<li>$1</li>')
    // Ordered lists
    .replace(/^\d+\. (.+)$/gm, '<li>$1</li>')
    // Blockquotes
    .replace(/^> (.+)$/gm, '<blockquote><p>$1</p></blockquote>')
    // Tables (basic)
    .replace(/^\|(.+)\|$/gm, (match) => {
      const cells = match.split('|').filter((c: string) => c.trim());
      if (cells.every((c: string) => /^[\s-:]+$/.test(c))) return '';
      return '<tr>' + cells.map((c: string) => `<td>${c.trim()}</td>`).join('') + '</tr>';
    })
    // Paragraphs (lines not already wrapped)
    .replace(/^(?!<[huptdol]|<li|<tr|<pre|<code|<blockquote|<hr|<strong|<em)(.+)$/gm, '<p>$1</p>')
    // Line breaks
    .replace(/\n\n/g, '\n');

  // Wrap consecutive <li> in <ul>
  html = html.replace(/((?:<li>.*<\/li>\n?)+)/g, '<ul>$1</ul>');
  // Wrap consecutive <tr> in <table>
  html = html.replace(/((?:<tr>.*<\/tr>\n?)+)/g, '<table>$1</table>');

  return html;
}

const docsDir = path.join(process.cwd(), '..', '..', 'docs');

export async function generateStaticParams() {
  try {
    const files = fs.readdirSync(docsDir).filter(f => f.endsWith('.md'));
    return files.map(f => ({ slug: f.replace('.md', '') }));
  } catch {
    return [];
  }
}

export default async function DocPage({ params }: PageProps) {
  const { slug } = await params;
  const filePath = path.join(docsDir, `${slug}.md`);

  let content = '';
  try {
    content = fs.readFileSync(filePath, 'utf-8');
  } catch {
    content = `# Document Not Found: ${slug}\n\nThe requested document could not be found.`;
  }

  const html = markdownToHtml(content);

  return (
    <article
      className="docs-content-body"
      dangerouslySetInnerHTML={{ __html: html }}
    />
  );
}
