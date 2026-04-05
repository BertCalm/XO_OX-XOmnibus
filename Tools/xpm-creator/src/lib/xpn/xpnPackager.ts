import JSZip from 'jszip';
import type { XpnPackageConfig } from './xpnTypes';
import { generateExpansionXml } from './expansionXml';

/**
 * Build an XPN expansion pack file (ZIP archive with .xpn extension).
 *
 * Directory structure:
 * ExpansionName/
 * ├── Content/
 * │   ├── Programs/
 * │   │   └── *.xpm
 * │   └── Samples/
 * │       └── *.WAV
 * └── Metadata/
 *     ├── Expansion.xml
 *     ├── cover.jpg/png
 *     └── preview.wav/mp3
 */
export async function buildXpnPackage(
  config: XpnPackageConfig,
  onProgress?: (step: string, progress: number) => void
): Promise<Blob> {
  const zip = new JSZip();
  const rootDir = sanitizeDirName(config.metadata.title);

  const totalItems =
    config.programs.length +
    config.samples.length +
    (config.coverArt ? 1 : 0) +
    2; // Expansion.xml + final

  let processed = 0;
  const reportProgress = (step: string) => {
    processed++;
    onProgress?.(step, (processed / totalItems) * 100);
  };

  // Generate and add Expansion.xml
  const expansionXml = generateExpansionXml(config.metadata);
  zip.file(`${rootDir}/Metadata/Expansion.xml`, expansionXml);
  reportProgress('Generated Expansion.xml');

  // Add cover art
  if (config.coverArt) {
    // Safety: strip MIME prefix if present then whitelist to jpg/png only
    const rawType = config.coverArt.type.replace(/^image\//, '').replace('jpeg', 'jpg');
    const ext = rawType === 'jpg' || rawType === 'jpeg' ? 'jpg' : 'png';
    zip.file(`${rootDir}/Metadata/cover.${ext}`, config.coverArt.data);
    reportProgress('Added cover art');
  }

  // Add preview audio
  if (config.previewAudio) {
    const VALID_AUDIO_TYPES = new Set(['wav', 'mp3']);
    const ext = VALID_AUDIO_TYPES.has(config.previewAudio.type) ? config.previewAudio.type : 'wav';
    zip.file(`${rootDir}/Metadata/preview.${ext}`, config.previewAudio.data);
  }

  // Add XPM program files — rewrite <SampleFile> paths so MPC firmware
  // resolves them relative to Content/Programs/ → Content/Samples/.
  // Without ../Samples/ prefix, the MPC looks for samples in Programs/.
  for (const program of config.programs) {
    const xpmFileName = sanitizeFileName(program.name) + '.xpm';
    let xpmContent = program.xpmContent;

    // Rewrite SampleFile references to use ../Samples/ relative path.
    // Extract the basename first to prevent path doubling — an imported XPM
    // might already have "Samples/Kick.WAV" which would become
    // "../Samples/Samples/Kick.WAV" without basename extraction.
    xpmContent = xpmContent.replace(
      /<SampleFile>(?!\.\.\/Samples\/)([^<]+)<\/SampleFile>/g,
      (_, p) => {
        const basename = p.split('/').pop()?.split('\\').pop() ?? p;
        return `<SampleFile>../Samples/${basename}</SampleFile>`;
      },
    );

    zip.file(`${rootDir}/Content/Programs/${xpmFileName}`, xpmContent);
    reportProgress(`Added program: ${program.name}`);
  }

  // Add sample WAV files — use STORE (no compression) for PCM audio.
  // WAV data is not compressible and some MPC firmware versions reject
  // DEFLATE-compressed audio files inside XPN packages.
  for (const sample of config.samples) {
    // Validate sample data before adding to ZIP — empty or detached
    // ArrayBuffers would produce corrupted entries.
    if (!sample.data || sample.data.byteLength === 0) {
      throw new Error(
        `Sample "${sample.fileName}" has no audio data. ` +
        'Re-import the sample and try exporting again.'
      );
    }
    const safeName = sample.fileName.split('/').pop()?.split('\\').pop() ?? sample.fileName;
    zip.file(`${rootDir}/Content/Samples/${safeName}`, sample.data, {
      compression: 'STORE',
    });
    reportProgress(`Added sample: ${sample.fileName}`);
  }

  // Add XO_OX creator metadata
  const metadata = {
    creator: 'XO_OX',
    version: '1.0.0',
    timestamp: new Date().toISOString(),
    programCount: config.programs.length,
    sampleCount: config.samples.length,
  };
  zip.file(`${rootDir}/Metadata/xo_ox_metadata.json`, JSON.stringify(metadata, null, 2));

  onProgress?.('Compressing...', 95);

  // Generate the ZIP blob — DEFLATE for text (XML/XPM), STORE already set for WAV above
  let blob: Blob;
  try {
    blob = await zip.generateAsync({
      type: 'blob',
      compression: 'DEFLATE',
      compressionOptions: { level: 6 },
    });
  } catch (error) {
    throw new Error(
      `XPN packaging failed during compression: ${error instanceof Error ? error.message : 'unknown error'}. ` +
      `Package contains ${config.samples.length} samples — try reducing sample count if this is a memory issue.`
    );
  }

  onProgress?.('Complete', 100);

  return blob;
}

/**
 * Download a blob as an XPN file
 */
export function downloadXpn(blob: Blob, fileName: string): void {
  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = fileName.endsWith('.xpn') ? fileName : `${fileName}.xpn`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  // Delay URL revocation — some browsers haven't finished initiating the
  // download when a.click() returns synchronously. Revoking too early can
  // produce a 0-byte file.
  setTimeout(() => URL.revokeObjectURL(url), 10000);
}

/**
 * Download an XPM file with its samples as a ZIP
 */
export async function downloadXpmWithSamples(
  xpmContent: string,
  xpmName: string,
  samples: { fileName: string; data: ArrayBuffer }[]
): Promise<void> {
  // Sanitize the name used for ZIP entry and download filename —
  // unsanitized names can contain path traversal or invalid characters.
  const safeName = sanitizeFileName(xpmName);
  const zip = new JSZip();

  zip.file(`${safeName}.xpm`, xpmContent);
  for (const sample of samples) {
    // Use STORE for WAV samples — PCM is not compressible and
    // some MPC firmware rejects DEFLATE-compressed audio
    const safeSampleName = sample.fileName.split('/').pop()?.split('\\').pop() ?? sample.fileName;
    zip.file(safeSampleName, sample.data, { compression: 'STORE' });
  }

  const blob = await zip.generateAsync({
    type: 'blob',
    compression: 'DEFLATE',
  });

  const url = URL.createObjectURL(blob);
  const a = document.createElement('a');
  a.href = url;
  a.download = `${safeName}.zip`;
  document.body.appendChild(a);
  a.click();
  document.body.removeChild(a);
  // Delay URL revocation to let the browser finish initiating the download
  setTimeout(() => URL.revokeObjectURL(url), 10000);
}

function sanitizeDirName(name: string): string {
  return name
    .replace(/[^a-zA-Z0-9_-]/g, '_')  // Replace spaces + special chars with underscore
    .replace(/_+/g, '_')               // Collapse consecutive underscores
    .replace(/^_|_$/g, '')             // Strip leading/trailing underscores
    || 'Expansion';                    // Fallback if name becomes empty
}

function sanitizeFileName(name: string): string {
  return name.replace(/[^a-zA-Z0-9_-]/g, '_').replace(/_+/g, '_').replace(/^_|_$/g, '') || 'Untitled';
}
