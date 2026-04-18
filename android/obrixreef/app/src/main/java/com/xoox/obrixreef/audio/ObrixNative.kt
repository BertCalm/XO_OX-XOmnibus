// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.audio

/**
 * JNI declarations matching obrix_jni.cpp native functions.
 * All methods are called via [ObrixBridge] — do not use directly.
 */
object ObrixNative {

    init {
        System.loadLibrary("obrixreef_jni")
    }

    // Lifecycle
    external fun startAudio(): Boolean
    external fun stopAudio()
    external fun isRunning(): Boolean

    // MIDI input
    external fun noteOn(note: Int, velocity: Float)
    external fun noteOff(note: Int)
    external fun allNotesOff()

    // Parameters
    external fun setParameter(index: Int, value: Float)
    external fun getParameter(index: Int): Float
    external fun getParameterCount(): Int
    external fun getParameterIdAt(index: Int): String

    // Preset loading
    external fun loadPresetJson(json: String)

    // Queries
    external fun getActiveVoiceCount(): Int
    external fun getSampleRate(): Double

    // Raw MIDI (from android.media.midi)
    external fun sendMidiBytes(data: ByteArray, offset: Int, count: Int)
}
