// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.audio

import android.util.Log

/**
 * Kotlin singleton wrapping the native Obrix audio engine.
 * Mirrors the ObrixBridge pattern from ObrixPocket (iOS).
 *
 * Thread safety: noteOn/noteOff/setParameter are lock-free (queue to audio thread).
 * start/stop must be called from the main thread.
 */
object ObrixBridge {

    private const val TAG = "ObrixBridge"

    // Parameter ID cache (built once at init)
    private val paramIds: List<String> by lazy {
        val count = ObrixNative.getParameterCount()
        (0 until count).map { ObrixNative.getParameterIdAt(it) }
    }

    private val paramIdToIndex: Map<String, Int> by lazy {
        paramIds.withIndex().associate { (i, id) -> id to i }
    }

    val parameterCount: Int get() = paramIds.size

    // --- Lifecycle ---

    fun start(): Boolean {
        val result = ObrixNative.startAudio()
        if (result) {
            Log.i(TAG, "Audio started at ${ObrixNative.getSampleRate()} Hz")
        } else {
            Log.e(TAG, "Failed to start audio")
        }
        return result
    }

    fun stop() {
        ObrixNative.stopAudio()
        Log.i(TAG, "Audio stopped")
    }

    val isRunning: Boolean get() = ObrixNative.isRunning()

    // --- MIDI ---

    fun noteOn(note: Int, velocity: Float) = ObrixNative.noteOn(note, velocity)
    fun noteOff(note: Int) = ObrixNative.noteOff(note)
    fun allNotesOff() = ObrixNative.allNotesOff()

    // --- Parameters ---

    fun setParameter(id: String, value: Float) {
        val index = paramIdToIndex[id] ?: return
        ObrixNative.setParameter(index, value)
    }

    fun setParameter(index: Int, value: Float) {
        ObrixNative.setParameter(index, value)
    }

    fun getParameter(id: String): Float {
        val index = paramIdToIndex[id] ?: return 0f
        return ObrixNative.getParameter(index)
    }

    fun getParameter(index: Int): Float = ObrixNative.getParameter(index)

    fun getParameterIdAt(index: Int): String = paramIds.getOrElse(index) { "" }

    fun parameterIndexOf(id: String): Int = paramIdToIndex[id] ?: -1

    // --- Macros (convenience) ---

    fun setMacroCharacter(value: Float) = setParameter("obrix_macroCharacter", value)
    fun setMacroMovement(value: Float) = setParameter("obrix_macroMovement", value)
    fun setMacroCoupling(value: Float) = setParameter("obrix_macroCoupling", value)
    fun setMacroSpace(value: Float) = setParameter("obrix_macroSpace", value)

    // --- Queries ---

    val activeVoiceCount: Int get() = ObrixNative.getActiveVoiceCount()
    val sampleRate: Double get() = ObrixNative.getSampleRate()
}
