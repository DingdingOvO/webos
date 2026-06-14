'use client';

import { useState, useEffect, useCallback } from 'react';

export type Lang = 'zh' | 'zh-TW' | 'en' | 'ja' | 'ko' | 'ru';
export type Theme = 'light' | 'dark';

const VALID_LANGS: Lang[] = ['zh', 'zh-TW', 'en', 'ja', 'ko', 'ru'];

interface UseThemeReturn {
  lang: Lang;
  theme: Theme;
  mounted: boolean;
  isDark: boolean;
  setLang: (lang: Lang) => void;
  toggleTheme: () => void;
}

export function useTheme(): UseThemeReturn {
  const [lang, setLangState] = useState<Lang>('zh');
  const [theme, setThemeState] = useState<Theme>('light');
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    const savedLang = localStorage.getItem('webos-lang') as Lang;
    const savedTheme = localStorage.getItem('webos-theme') as Theme;
    if (savedLang && VALID_LANGS.includes(savedLang)) setLangState(savedLang);
    if (savedTheme && ['light', 'dark'].includes(savedTheme)) setThemeState(savedTheme);
    setMounted(true);
  }, []);

  useEffect(() => {
    if (mounted) {
      document.documentElement.setAttribute('data-theme', theme);
    }
  }, [theme, mounted]);

  const setLang = useCallback((newLang: Lang) => {
    setLangState(newLang);
    localStorage.setItem('webos-lang', newLang);
  }, []);

  const toggleTheme = useCallback(() => {
    setThemeState((prev) => {
      const next = prev === 'dark' ? 'light' : 'dark';
      localStorage.setItem('webos-theme', next);
      return next;
    });
  }, []);

  return {
    lang,
    theme,
    mounted,
    isDark: theme === 'dark',
    setLang,
    toggleTheme,
  };
}
