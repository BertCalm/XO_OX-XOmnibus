// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.glyph

import android.util.Log
import com.xoox.obrixreef.audio.ObrixBridge

/**
 * Maps Nothing Phone (3) Glyph Matrix touch zones to MIDI notes.
 *
 * The 25x25 Glyph Matrix on Phone (3) supports touch input.
 * This mapper divides the grid into 8 zones (4x2), each triggering
 * a different note — turning the back of the phone into a synth pad.
 *
 * Zone layout (back of phone, viewed from behind):
 *   ┌──────┬──────┬──────┬──────┐
 *   │  C4  │  D4  │  E4  │  F4  │
 *   ├──────┼──────┼──────┼──────┤
 *   │  G4  │  A4  │  B4  │  C5  │
 *   └──────┴──────┴──────┴──────┘
 *
 * Touch events arrive via the Glyph Matrix SDK's touch callback.
 * Uses reflection to avoid compile-time SDK dependency.
 */
class GlyphTouchMapper {

    companion object {
        private const val TAG = "GlyphTouchMapper"
        private const val GRID_SIZE = 25
        private const val COLS = 4
        private const val ROWS = 2
    }

    // C major scale across the 8 zones
    private val zoneNotes = intArrayOf(60, 62, 64, 65, 67, 69, 71, 72)

    // Track which zones are currently pressed
    private val activeZones = BooleanArray(COLS * ROWS)

    /**
     * Handle a Glyph Matrix touch event.
     * @param x Grid column (0..24)
     * @param y Grid row (0..24)
     * @param pressed true = touch down, false = touch up
     */
    fun onGlyphTouch(x: Int, y: Int, pressed: Boolean) {
        val zoneCol = (x * COLS) / GRID_SIZE
        val zoneRow = (y * ROWS) / GRID_SIZE
        val zoneIndex = zoneRow * COLS + zoneCol

        if (zoneIndex < 0 || zoneIndex >= zoneNotes.size) return

        if (pressed && !activeZones[zoneIndex]) {
            activeZones[zoneIndex] = true
            ObrixBridge.noteOn(zoneNotes[zoneIndex], 0.8f)
            Log.d(TAG, "Glyph zone $zoneIndex ON → note ${zoneNotes[zoneIndex]}")
        } else if (!pressed && activeZones[zoneIndex]) {
            activeZones[zoneIndex] = false
            ObrixBridge.noteOff(zoneNotes[zoneIndex])
            Log.d(TAG, "Glyph zone $zoneIndex OFF → note ${zoneNotes[zoneIndex]}")
        }
    }

    /**
     * Release all active zones (called on stop / orientation change).
     */
    fun releaseAll() {
        for (i in activeZones.indices) {
            if (activeZones[i]) {
                activeZones[i] = false
                ObrixBridge.noteOff(zoneNotes[i])
            }
        }
    }

    /**
     * Set the scale for the zone grid.
     * @param rootNote MIDI note for zone 0
     * @param intervals Semitone offsets from root for each subsequent zone
     */
    fun setScale(rootNote: Int, intervals: IntArray) {
        for (i in zoneNotes.indices) {
            zoneNotes[i] = if (i < intervals.size) rootNote + intervals[i] else rootNote + i * 2
        }
    }
}
