import { parseXpmXml } from '../xpm/xpmParser';
import type { ParsedXpmKit } from '../xpm/xpmParser';

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

export interface ExtractedExpansion {
  /** Expansion metadata from Expansion.xml */
  metadata: {
    title: string;
    identifier: string;
    manufacturer: string;
    description: string;
    version: string;
  };
  /** All programs found in the expansion */
  programs: { name: string; kit: ParsedXpmKit; rawXml: string }[];
  /** All sample files (filename -> ArrayBuffer) */
  samples: Map<string, ArrayBuffer>;
  /** Cover art if present */
  coverArt?: { data: ArrayBuffer; type: 'jpg' | 'png' };
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

function getTextFromXml(el: Element, tag: string, defaultVal = ''): string {
  const child = el.getElementsByTagName(tag)[0];
  return child?.textContent?.trim() ?? defaultVal;
}

function parseExpansionXml(xmlString: string): ExtractedExpansion['metadata'] {
  const parser = new DOMParser();
  const doc = parser.parseFromString(xmlString, 'application/xml');

  const root = doc.getElementsByTagName('Expansion')[0];
  if (!root) {
    return {
      title: 'Unknown Expansion',
      identifier: '',
      manufacturer: '',
      description: '',
      version: '1.0',
    };
  }

  return {
    title: getTextFromXml(root, 'Title', 'Unknown Expansion'),
    identifier: getTextFromXml(root, 'Identifier'),
    manufacturer: getTextFromXml(root, 'Creator', getTextFromXml(root, 'Manufacturer')),
    description: getTextFromXml(root, 'Description'),
    version: getTextFromXml(root, 'Version', '1.0'),
  };
}

/**
 * Extract the filename from a full zip path.
 * e.g. "MyExpansion/Content/Samples/Kick.WAV" -> "Kick.WAV"
 */
function filenameFromPath(zipPath: string): string {
  const parts = zipPath.split('/');
  return parts[parts.length - 1];
}

// ---------------------------------------------------------------------------
// Main extractor
// ---------------------------------------------------------------------------

/**
 * Extract and parse an XPN file (which is a ZIP archive).
 *
 * Handles the case where JSZip might not be available by doing a dynamic
 * import, so the module can be tree-shaken or lazily loaded.
 */
export async function extractXpn(
  xpnBuffer: ArrayBuffer,
  onProgress?: (step: string, pct: number) => void,
): Promise<ExtractedExpansion> {
  // Dynamic import of JSZip for tree-shaking / lazy load support
  // eslint-disable-next-line @typescript-eslint/no-explicit-any
  let JSZipModule: any;
  try {
    JSZipModule = await import('jszip');
  } catch {
    throw new Error(
      'JSZip is required for XPN extraction. Install it with: npm install jszip',
    );
  }

  // Handle both ESM default export and CJS module
  const JSZipConstructor = JSZipModule.default ?? JSZipModule;

  onProgress?.('Opening XPN archive...', 5);

  const zip = await JSZipConstructor.loadAsync(xpnBuffer) as import('jszip');

  // Collect all file paths
  const allPaths: string[] = [];
  zip.forEach((relativePath) => {
    allPaths.push(relativePath);
  });

  const totalFiles = allPaths.length;
  let processedFiles = 0;

  const reportFileProgress = (step: string) => {
    processedFiles++;
    const basePct = 10; // after opening
    const filePct = 85; // file processing range
    const pct = basePct + (processedFiles / Math.max(1, totalFiles)) * filePct;
    onProgress?.(step, Math.min(95, pct));
  };

  // ---------------------------------------------------------------------------
  // 1. Find and parse Expansion.xml
  // ---------------------------------------------------------------------------
  onProgress?.('Reading expansion metadata...', 10);

  let metadata: ExtractedExpansion['metadata'] = {
    title: 'Unknown Expansion',
    identifier: '',
    manufacturer: '',
    description: '',
    version: '1.0',
  };

  const expansionXmlPath = allPaths.find(
    (p) => p.toLowerCase().endsWith('expansion.xml'),
  );

  if (expansionXmlPath) {
    const file = zip.file(expansionXmlPath);
    if (file) {
      const xmlString = await file.async('string');
      metadata = parseExpansionXml(xmlString);
      reportFileProgress('Parsed Expansion.xml');
    }
  }

  // ---------------------------------------------------------------------------
  // 2. Find and parse all .xpm programs
  // ---------------------------------------------------------------------------
  const xpmPaths = allPaths.filter((p) =>
    p.toLowerCase().endsWith('.xpm'),
  );

  const programs: ExtractedExpansion['programs'] = [];

  for (const xpmPath of xpmPaths) {
    const file = zip.file(xpmPath);
    if (!file) continue;

    const rawXml = await file.async('string');
    try {
      const kit = parseXpmXml(rawXml);
      const name = kit.name || filenameFromPath(xpmPath).replace(/\.xpm$/i, '');
      programs.push({ name, kit, rawXml });
      reportFileProgress(`Parsed program: ${name}`);
    } catch (err) {
      // Skip malformed XPM files but log for debugging
      console.warn(`[xpnExtractor] Failed to parse XPM: ${xpmPath}`, err);
      reportFileProgress(`Skipped malformed: ${filenameFromPath(xpmPath)}`);
    }
  }

  // ---------------------------------------------------------------------------
  // 3. Collect all sample files (.wav, .WAV, .aif, .aiff)
  // ---------------------------------------------------------------------------
  const sampleExtensions = ['.wav', '.aif', '.aiff'];
  const samplePaths = allPaths.filter((p) => {
    const lower = p.toLowerCase();
    return sampleExtensions.some((ext) => lower.endsWith(ext));
  });

  const samples = new Map<string, ArrayBuffer>();

  for (const samplePath of samplePaths) {
    const file = zip.file(samplePath);
    if (!file) continue;

    const data = await file.async('arraybuffer');
    const fileName = filenameFromPath(samplePath);
    samples.set(fileName, data);
    reportFileProgress(`Extracted sample: ${fileName}`);
  }

  // ---------------------------------------------------------------------------
  // 4. Look for cover art
  // ---------------------------------------------------------------------------
  let coverArt: ExtractedExpansion['coverArt'] | undefined;

  // Find cover art by checking directories (Content/ or Metadata/) and
  // filenames starting with 'cover'. Previous logic checked filename.startsWith('content')
  // which never matched real filenames — fixed to check full path instead.
  const uniqueCoverPaths = allPaths.filter((p) => {
    const lower = p.toLowerCase();
    const isImage = lower.endsWith('.png') || lower.endsWith('.jpg') || lower.endsWith('.jpeg');
    if (!isImage) return false;
    const name = filenameFromPath(lower);
    return (
      lower.includes('/content/') ||
      lower.includes('/metadata/') ||
      name.startsWith('cover')
    );
  });

  if (uniqueCoverPaths.length > 0) {
    const coverFile = zip.file(uniqueCoverPaths[0]);
    if (coverFile) {
      const data = await coverFile.async('arraybuffer');
      const isJpg =
        uniqueCoverPaths[0].toLowerCase().endsWith('.jpg') ||
        uniqueCoverPaths[0].toLowerCase().endsWith('.jpeg');
      coverArt = { data, type: isJpg ? 'jpg' : 'png' };
      reportFileProgress('Extracted cover art');
    }
  }

  onProgress?.('Extraction complete', 100);

  return {
    metadata,
    programs,
    samples,
    coverArt,
  };
}

/**
 * Read a single .xpm file from an ArrayBuffer (plain text XML).
 * Convenience wrapper for importing standalone XPM files.
 */
export async function extractSingleXpm(
  buffer: ArrayBuffer,
): Promise<{ kit: ParsedXpmKit; rawXml: string }> {
  const decoder = new TextDecoder('utf-8');
  const rawXml = decoder.decode(buffer);
  const kit = parseXpmXml(rawXml);
  return { kit, rawXml };
}
