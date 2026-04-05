// XPN Format Specification — TypeScript bridge
// Loaded from shared xpn-spec.json (single source of truth for Python + TypeScript).
// Import: import { xpnSpec } from '@/lib/xpn-spec'
//
// The JSON file is the cross-language authority.  This module is the TypeScript API;
// do not hardcode velocity zones, tier budgets, or voice tables anywhere else in the
// xpm-creator source.  If a value needs changing, update xpn-spec.json and both
// languages pick it up automatically.

import specData from '../../../xpn-spec.json'

export const xpnSpec = specData

export type VelocityZone = typeof specData.velocity.zones[number]
export type TierName = keyof typeof specData.tiers
export type VoiceName = typeof specData.voices.onset.voices[number]

/**
 * Return the velocity zone descriptor for a given MIDI velocity (1–127).
 * Defaults to the Hard zone when the value falls outside all defined ranges
 * (e.g. vel === 0 or vel > 127).
 */
export const getVelocityZone = (velocity: number): VelocityZone => {
  return (
    specData.velocity.zones.find(
      (z) => velocity >= z.velStart && velocity <= z.velEnd
    ) ?? specData.velocity.zones[specData.velocity.zones.length - 1] // Hard
  )
}

/**
 * Return the maximum XPM instrument-slot count for a given tier.
 */
export const getSlotBudget = (tier: TierName): number => {
  return specData.tiers[tier].maxSlots
}

/**
 * Return the render midpoint velocity for a layer index (0 = Ghost … 3 = Hard).
 * Clamps to the last zone for out-of-range indices.
 */
export const getRenderMidpoint = (layerIndex: number): number => {
  const zones = specData.velocity.zones
  const zone = zones[Math.min(layerIndex, zones.length - 1)]
  return zone.renderMidpoint
}

/**
 * Return the round-robin count for an ONSET voice (engine-internal name, e.g. "chat").
 * Returns 0 when the voice is not found (meaning velocity-only, no RR).
 */
export const getOnsetRRCount = (voiceName: VoiceName): number => {
  return (specData.voices.onset.roundRobinCounts as Record<string, number>)[voiceName] ?? 0
}

/**
 * Return the GM MIDI note for an ONSET voice.
 */
export const getOnsetMidiNote = (voiceName: VoiceName): number => {
  return (specData.voices.onset.midiNotes as Record<string, number>)[voiceName] ?? 36
}

/**
 * Return the XPN canonical pad name for an ONSET engine-internal voice name.
 * e.g. "chat" → "closed_hat", "ohat" → "open_hat".
 */
export const getOnsetCanonicalName = (voiceName: VoiceName): string => {
  return (specData.voices.onset.canonicalNames as Record<string, string>)[voiceName] ?? voiceName
}
