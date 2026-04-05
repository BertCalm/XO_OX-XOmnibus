/**
 * Input sanitization helpers shared across the app.
 *
 * Keep this module free of side effects — pure functions only.
 */

/**
 * Validate and sanitize a user-supplied name (project name, program name, etc.).
 * Strips ASCII control characters and enforces a maximum length.
 *
 * @param name   Raw string from user input.
 * @param maxLen Maximum allowed length after stripping controls (default 255).
 * @returns      Sanitized string, trimmed.
 */
export function validateName(name: string, maxLen = 255): string {
  // eslint-disable-next-line no-control-regex
  return name.replace(/[\x00-\x1f\x7f]/g, '').slice(0, maxLen).trim();
}

/**
 * Sanitize a filename from an untrusted source (e.g. a ZIP entry) to prevent
 * path traversal.  Strips `..` segments, leading slashes, and returns only
 * the final path component.
 */
export function sanitizeFilename(name: string): string {
  return name.replace(/\.\./g, '').replace(/^\/+/, '').split('/').pop() || 'unnamed';
}

/**
 * Strip URLs and long opaque tokens from an error message before displaying
 * it in the UI.  Prevents API keys, OAuth tokens, and endpoint URLs from
 * leaking to the user (or to browser devtools via React state).
 */
export function sanitizeErrorMessage(msg: string): string {
  return msg
    .replace(/https?:\/\/[^\s]+/g, '[URL]')
    .replace(/[A-Za-z0-9]{32,}/g, '[TOKEN]');
}
