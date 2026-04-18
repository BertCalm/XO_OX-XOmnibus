// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.glyph

import android.content.Context
import android.hardware.Sensor
import android.hardware.SensorEvent
import android.hardware.SensorEventListener
import android.hardware.SensorManager
import android.util.Log

/**
 * Detects when the phone is placed face-down using proximity sensor
 * and accelerometer. When face-down, the Glyph Matrix becomes the
 * primary display and Reef Aquarium mode activates.
 *
 * Detection logic:
 *   1. Proximity sensor reports near (< 1cm) → screen side is blocked
 *   2. Accelerometer Z-axis is negative (gravity pulling away from screen)
 *   3. Both conditions stable for 500ms → face-down confirmed
 */
class FaceDownDetector(context: Context) {

    companion object {
        private const val TAG = "FaceDownDetector"
        private const val STABILITY_MS = 500L
        private const val GRAVITY_THRESHOLD = -7.0f  // ~70% of 9.8 m/s²
    }

    interface Listener {
        fun onFaceDownChanged(isFaceDown: Boolean)
    }

    var listener: Listener? = null

    private val sensorManager = context.getSystemService(Context.SENSOR_SERVICE) as SensorManager
    private val proximitySensor = sensorManager.getDefaultSensor(Sensor.TYPE_PROXIMITY)
    private val accelerometer = sensorManager.getDefaultSensor(Sensor.TYPE_ACCELEROMETER)

    private var isProximityNear = false
    private var isGravityFlipped = false
    private var isFaceDown = false
    private var stableStartTime = 0L

    private val sensorListener = object : SensorEventListener {
        override fun onSensorChanged(event: SensorEvent) {
            when (event.sensor.type) {
                Sensor.TYPE_PROXIMITY -> {
                    val maxRange = event.sensor.maximumRange
                    isProximityNear = event.values[0] < maxRange * 0.5f
                    evaluate()
                }
                Sensor.TYPE_ACCELEROMETER -> {
                    val z = event.values[2]
                    isGravityFlipped = z < GRAVITY_THRESHOLD
                    evaluate()
                }
            }
        }

        override fun onAccuracyChanged(sensor: Sensor?, accuracy: Int) {}
    }

    fun start() {
        proximitySensor?.let {
            sensorManager.registerListener(sensorListener, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
        accelerometer?.let {
            sensorManager.registerListener(sensorListener, it, SensorManager.SENSOR_DELAY_NORMAL)
        }
        Log.i(TAG, "Face-down detection started")
    }

    fun stop() {
        sensorManager.unregisterListener(sensorListener)
        if (isFaceDown) {
            isFaceDown = false
            listener?.onFaceDownChanged(false)
        }
        Log.i(TAG, "Face-down detection stopped")
    }

    private fun evaluate() {
        val candidateFaceDown = isProximityNear && isGravityFlipped
        val now = System.currentTimeMillis()

        if (candidateFaceDown) {
            if (stableStartTime == 0L) {
                stableStartTime = now
            } else if (now - stableStartTime >= STABILITY_MS && !isFaceDown) {
                isFaceDown = true
                Log.i(TAG, "Face-down DETECTED — entering Reef Aquarium mode")
                listener?.onFaceDownChanged(true)
            }
        } else {
            stableStartTime = 0L
            if (isFaceDown) {
                isFaceDown = false
                Log.i(TAG, "Face-up DETECTED — exiting Reef Aquarium mode")
                listener?.onFaceDownChanged(false)
            }
        }
    }
}
