// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.audio

import android.content.Context
import android.media.midi.MidiDevice
import android.media.midi.MidiDeviceInfo
import android.media.midi.MidiManager
import android.media.midi.MidiOutputPort
import android.media.midi.MidiReceiver
import android.os.Handler
import android.os.Looper
import android.util.Log

/**
 * Manages USB and Bluetooth MIDI input via android.media.midi.
 * Forwards raw MIDI bytes to the native engine via JNI.
 */
class MidiInputHandler(context: Context) {

    private val TAG = "MidiInputHandler"
    private val midiManager = context.getSystemService(Context.MIDI_SERVICE) as? MidiManager
    private val handler = Handler(Looper.getMainLooper())
    private val openDevices = mutableListOf<MidiDevice>()
    private val openPorts = mutableListOf<MidiOutputPort>()

    private val receiver = object : MidiReceiver() {
        override fun onSend(msg: ByteArray, offset: Int, count: Int, timestamp: Long) {
            // Forward raw MIDI to native engine
            ObrixNative.sendMidiBytes(msg, offset, count)
        }
    }

    private val deviceCallback = object : MidiManager.DeviceCallback() {
        override fun onDeviceAdded(info: MidiDeviceInfo) {
            Log.i(TAG, "MIDI device added: ${info.properties}")
            openDevice(info)
        }

        override fun onDeviceRemoved(info: MidiDeviceInfo) {
            Log.i(TAG, "MIDI device removed: ${info.properties}")
            // Cleanup happens in closeAll()
        }
    }

    fun start() {
        midiManager?.registerDeviceCallback(deviceCallback, handler)

        // Connect to any already-present devices
        midiManager?.devices?.forEach { info ->
            if (info.outputPortCount > 0) {
                openDevice(info)
            }
        }

        Log.i(TAG, "MIDI input handler started")
    }

    fun stop() {
        midiManager?.unregisterDeviceCallback(deviceCallback)
        closeAll()
        Log.i(TAG, "MIDI input handler stopped")
    }

    private fun openDevice(info: MidiDeviceInfo) {
        if (info.outputPortCount == 0) return

        midiManager?.openDevice(info, { device ->
            if (device != null) {
                openDevices.add(device)

                // Open all output ports (output from the MIDI device = input to us)
                for (i in 0 until info.outputPortCount) {
                    val port = device.openOutputPort(i)
                    if (port != null) {
                        port.connect(receiver)
                        openPorts.add(port)
                        Log.i(TAG, "Connected to MIDI port $i on ${info.properties}")
                    }
                }
            }
        }, handler)
    }

    private fun closeAll() {
        openPorts.forEach { port ->
            port.disconnect(receiver)
            port.close()
        }
        openPorts.clear()

        openDevices.forEach { it.close() }
        openDevices.clear()
    }
}
