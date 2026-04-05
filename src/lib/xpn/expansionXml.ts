import type { XpnMetadata } from './xpnTypes';

/**
 * Generate the Expansion.xml metadata file for an XPN package.
 */
export function generateExpansionXml(metadata: XpnMetadata): string {
  const lines: string[] = [];

  lines.push('<?xml version="1.0" encoding="UTF-8"?>');
  lines.push('<Expansion>');
  lines.push(`  <Title>${escapeXml(metadata.title)}</Title>`);
  lines.push(`  <Identifier>${escapeXml(metadata.identifier)}</Identifier>`);
  lines.push(`  <Description>${escapeXml(metadata.description)}</Description>`);
  lines.push(`  <Creator>${escapeXml(metadata.creator)}</Creator>`);
  lines.push(`  <Version>${escapeXml(metadata.version)}</Version>`);

  if (metadata.tags.length > 0) {
    lines.push('  <Tags>');
    for (const tag of metadata.tags) {
      lines.push(`    <Tag>${escapeXml(tag)}</Tag>`);
    }
    lines.push('  </Tags>');
  }

  lines.push('</Expansion>');

  return lines.join('\n');
}

function escapeXml(str: string): string {
  return str
    .replace(/&/g, '&amp;')
    .replace(/</g, '&lt;')
    .replace(/>/g, '&gt;')
    .replace(/"/g, '&quot;')
    .replace(/'/g, '&apos;');
}
