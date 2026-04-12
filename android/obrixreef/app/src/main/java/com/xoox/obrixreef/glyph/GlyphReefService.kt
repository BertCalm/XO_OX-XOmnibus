// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.glyph

import android.content.ComponentName
import android.content.Context
import android.content.Intent
import android.content.ServiceConnection
import android.os.IBinder
import android.util.Log
import com.xoox.obrixreef.audio.ObrixBridge
import kotlinx.coroutines.*
import kotlin.math.abs
import kotlin.math.sin

/**
 * Glyph Matrix reef visualization for Nothing Phone (3).
 *
 * Uses the Nothing Glyph Developer Kit to drive the 25x25 (489 LED)
 * rear matrix as an audio-reactive reef display.
 *
 * 3-layer composition per the Glyph Matrix SDK:
 *   Layer 0: Background wave animation (slow sine sweep)
 *   Layer 1: Voice activity indicators (coral sprites)
 *   Layer 2: Audio-reactive brightness overlay
 *
 * Falls back gracefully on non-Nothing devices (no-op).
 */
class GlyphReefService(private val context: Context) {

    companion object {
        private const val TAG = "GlyphReefService"
        private const val GRID_SIZE = 25
        private const val TOTAL_LEDS = GRID_SIZE * GRID_SIZE  // 625 (489 physically addressable)
        private const val FRAME_INTERVAL_MS = 33L  // ~30fps for LED refresh
        private const val MAX_BRIGHTNESS = 4095
    }

    private var scope: CoroutineScope? = null
    private var isRunning = false
    private var glyphAvailable = false

    // Frame buffer (brightness per LED, 0..4095)
    private val frameBuffer = IntArray(TOTAL_LEDS)

    // Animation state
    private var wavePhase = 0.0
    private var frameCount = 0L

    /**
     * Start the Glyph reef visualization loop.
     * Safe to call on non-Nothing devices (detects and becomes no-op).
     */
    fun start() {
        if (isRunning) return

        glyphAvailable = isNothingDevice()
        if (!glyphAvailable) {
            Log.i(TAG, "Not a Nothing device — Glyph reef disabled")
            return
        }

        isRunning = true
        scope = CoroutineScope(Dispatchers.Default + SupervisorJob())

        scope?.launch {
            Log.i(TAG, "Glyph reef visualization started")

            while (isActive && isRunning) {
                compositeFrame()
                submitFrame()
                delay(FRAME_INTERVAL_MS)
            }
        }
    }

    /**
     * Stop the visualization and release Glyph resources.
     */
    fun stop() {
        isRunning = false
        scope?.cancel()
        scope = null

        // Clear the matrix
        if (glyphAvailable) {
            frameBuffer.fill(0)
            submitFrame()
        }

        Log.i(TAG, "Glyph reef visualization stopped")
    }

    /**
     * Composite all 3 layers into the frame buffer.
     */
    private fun compositeFrame() {
        frameBuffer.fill(0)
        frameCount++

        // Layer 0: Background wave
        renderWaveLayer()

        // Layer 1: Voice indicators
        renderVoiceLayer()

        // Layer 2: Audio-reactive overlay
        renderAudioReactiveLayer()
    }

    /**
     * Layer 0: Slow horizontal sine wave sweep (ocean current).
     * Very low brightness — sets the ambient mood.
     */
    private fun renderWaveLayer() {
        wavePhase += 0.02

        for (y in 0 until GRID_SIZE) {
            for (x in 0 until GRID_SIZE) {
                val wave = sin(wavePhase + x * 0.3 + y * 0.15)
                val brightness = ((wave + 1.0) * 0.5 * 200).toInt()  // 0..200 (dim)
                frameBuffer[y * GRID_SIZE + x] += brightness
            }
        }
    }

    /**
     * Layer 1: Active voice indicators as small coral blobs.
     * Each voice lights a cluster of LEDs at a fixed grid position.
     */
    private fun renderVoiceLayer() {
        val voiceCount = ObrixBridge.activeVoiceCount

        // 8 voice positions arranged in a reef pattern
        val voicePositions = arrayOf(
            4 to 4, 10 to 3, 16 to 5, 20 to 4,
            3 to 12, 9 to 14, 15 to 11, 21 to 13
        )

        for (v in 0 until minOf(voiceCount, 8)) {
            val (cx, cy) = voicePositions[v]
            // 3x3 coral blob
            for (dy in -1..1) {
                for (dx in -1..1) {
                    val x = cx + dx
                    val y = cy + dy
                    if (x in 0 until GRID_SIZE && y in 0 until GRID_SIZE) {
                        val dist = abs(dx) + abs(dy)
                        val brightness = if (dist == 0) 2000 else 1000
                        // Pulse with frame count for bioluminescence
                        val pulse = (sin(frameCount * 0.1 + v * 0.7) + 1.0) * 0.5
                        frameBuffer[y * GRID_SIZE + x] += (brightness * pulse).toInt()
                    }
                }
            }
        }
    }

    /**
     * Layer 2: Global brightness modulation from audio output.
     * All LEDs brighten proportional to the engine's signal level.
     */
    private fun renderAudioReactiveLayer() {
        // Use macro values as a proxy for audio activity
        // (direct audio level requires an additional JNI call — Phase 3B)
        val character = ObrixBridge.getParameter("obrix_macroCharacter")
        val movement = ObrixBridge.getParameter("obrix_macroMovement")
        val activity = (character + movement) * 0.5f

        if (activity > 0.01f) {
            val boost = (activity * 800).toInt()
            for (i in frameBuffer.indices) {
                frameBuffer[i] += boost
            }
        }

        // Clamp all values
        for (i in frameBuffer.indices) {
            frameBuffer[i] = frameBuffer[i].coerceIn(0, MAX_BRIGHTNESS)
        }
    }

    /**
     * Submit the frame buffer to the Glyph Matrix hardware.
     *
     * Uses reflection to call the Nothing Glyph SDK so we compile
     * without the SDK as a hard dependency. On non-Nothing devices
     * or if the SDK isn't available, this is a no-op.
     */
    private fun submitFrame() {
        if (!glyphAvailable) return

        try {
            // Nothing Glyph Matrix SDK call:
            //   GlyphManager.getInstance(context).setMatrixFrame(frameBuffer)
            //
            // Using reflection to avoid compile-time dependency on the SDK.
            // When the Nothing Glyph SDK AAR is added to the project,
            // replace this with direct API calls.
            val glyphManagerClass = Class.forName("com.nothing.ketchum.GlyphManager")
            val getInstance = glyphManagerClass.getMethod("getInstance", Context::class.java)
            val manager = getInstance.invoke(null, context)
            val setFrame = glyphManagerClass.getMethod("setMatrixFrame", IntArray::class.java)
            setFrame.invoke(manager, frameBuffer)
        } catch (e: ClassNotFoundException) {
            // Glyph SDK not available — disable further attempts
            glyphAvailable = false
            Log.w(TAG, "Glyph SDK not found — disabling Glyph reef")
        } catch (e: Exception) {
            Log.w(TAG, "Glyph frame submit failed: ${e.message}")
        }
    }

    private fun isNothingDevice(): Boolean {
        return android.os.Build.MANUFACTURER.equals("Nothing", ignoreCase = true)
    }
}
