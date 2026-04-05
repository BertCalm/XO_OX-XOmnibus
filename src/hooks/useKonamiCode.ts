import { useEffect, useRef } from 'react';

const KONAMI_CODE = [
  'ArrowUp',
  'ArrowUp',
  'ArrowDown',
  'ArrowDown',
  'ArrowLeft',
  'ArrowRight',
  'ArrowLeft',
  'ArrowRight',
  'KeyB',
  'KeyA',
];

/**
 * Listens for the classic Konami code key sequence.
 * When the full sequence is entered, fires the provided callback.
 */
export function useKonamiCode(callback: () => void) {
  const indexRef = useRef(0);

  useEffect(() => {
    function handleKeyDown(e: KeyboardEvent) {
      // Don't intercept when typing in input fields
      const target = e.target as HTMLElement;
      if (
        target.tagName === 'INPUT' ||
        target.tagName === 'TEXTAREA' ||
        target.tagName === 'SELECT' ||
        target.isContentEditable
      ) {
        return;
      }

      if (e.code === KONAMI_CODE[indexRef.current]) {
        indexRef.current++;
        if (indexRef.current === KONAMI_CODE.length) {
          indexRef.current = 0;
          callback();
        }
      } else {
        // Reset unless the key pressed is the start of the sequence
        indexRef.current = e.code === KONAMI_CODE[0] ? 1 : 0;
      }
    }

    window.addEventListener('keydown', handleKeyDown);
    return () => window.removeEventListener('keydown', handleKeyDown);
  }, [callback]);
}
