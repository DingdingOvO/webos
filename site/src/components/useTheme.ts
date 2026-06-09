'use client';

import { useState, useEffect, useCallback } from 'react';

export type Lang = 'zh' | 'zh-TW' | 'en' | 'ja' | 'ko' | 'ru';
export type Theme = 'light' | 'dark';

const LANG_KEY = 'webos-lang';
const THEME_KEY = 'webos-theme';

const LANGS: Lang[] = ['zh', 'zh-TW', 'en', 'ja', 'ko', 'ru'];

export function useTheme() {
  const [lang, setLangState] = useState<Lang>('zh');
  const [theme, setThemeState] = useState<Theme>('light');
  const [mounted, setMounted] = useState(false);

  useEffect(() => {
    const savedLang = (localStorage.getItem(LANG_KEY) as Lang) || 'zh';
    const savedTheme = (localStorage.getItem(THEME_KEY) as Theme) || 'light';

    setLangState(LANGS.includes(savedLang) ? savedLang : 'zh');
    setThemeState(savedTheme === 'dark' ? 'dark' : 'light');
    setMounted(true);
  }, []);

  useEffect(() => {
    if (!mounted) return;
    document.documentElement.setAttribute('data-theme', theme);
    localStorage.setItem(THEME_KEY, theme);
  }, [theme, mounted]);

  const setLang = useCallback((newLang: Lang) => {
    setLangState(newLang);
    localStorage.setItem(LANG_KEY, newLang);
  }, []);

  const toggleTheme = useCallback(() => {
    setThemeState((prev) => (prev === 'dark' ? 'light' : 'dark'));
  }, []);

  return {
    lang,
    setLang,
    theme,
    toggleTheme,
    mounted,
  };
}
