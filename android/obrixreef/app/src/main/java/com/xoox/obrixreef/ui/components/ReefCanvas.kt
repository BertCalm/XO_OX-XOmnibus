// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.ui.components

import androidx.compose.foundation.Canvas
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableLongStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.geometry.Offset
import androidx.compose.ui.graphics.Brush
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.Path
import androidx.compose.ui.graphics.drawscope.DrawScope
import kotlinx.coroutines.delay
import kotlin.math.sin

// Reef color palette
private val DeepWater = Color(0xFF0A1A2E)
private val MidWater = Color(0xFF0F2B3F)
private val ShallowWater = Color(0xFF143D4F)
private val ReefJade = Color(0xFF1E8B7E)
private val CoralPink = Color(0xFF8B3A5E)
private val BioGlow = Color(0xFF2ED8C4)

/**
 * Animated reef background using Compose Canvas.
 * 3-layer parallax with procedural coral silhouettes and particle bubbles.
 *
 * Layers (back to front):
 *   0: Deep water gradient (static, 0.1x scroll)
 *   1: Mid-water kelp forest (slow sway, 0.4x scroll)
 *   2: Foreground coral ridges (wave motion, 1.0x scroll)
 *   +: Bubble particles rising
 */
@Composable
fun ReefCanvas(
    modifier: Modifier = Modifier,
    activeVoices: Int = 0
) {
    var frameTime by remember { mutableLongStateOf(0L) }

    LaunchedEffect(Unit) {
        while (true) {
            frameTime = System.currentTimeMillis()
            delay(16) // ~60fps
        }
    }

    Canvas(modifier = modifier) {
        val t = frameTime / 1000.0
        val w = size.width
        val h = size.height

        // Layer 0: Deep water gradient
        drawRect(
            brush = Brush.verticalGradient(
                colors = listOf(DeepWater, MidWater, ShallowWater),
                startY = 0f,
                endY = h
            )
        )

        // Layer 1: Mid-water kelp (3 strands, slow sway)
        drawKelpForest(t * 0.4, w, h, activeVoices)

        // Layer 2: Coral ridge silhouettes along bottom
        drawCoralRidge(t, w, h)

        // Particles: rising bubbles
        drawBubbles(t, w, h, activeVoices)

        // Bioluminescence glow spots (tied to voice count)
        drawBioGlow(t, w, h, activeVoices)
    }
}

private fun DrawScope.drawKelpForest(t: Double, w: Float, h: Float, voices: Int) {
    val kelpPositions = floatArrayOf(0.15f, 0.45f, 0.75f)
    val kelpColor = Color(0xFF0D3D2E)

    for ((i, xFrac) in kelpPositions.withIndex()) {
        val baseX = w * xFrac
        val swayAmplitude = 15f + voices * 2f
        val path = Path()

        path.moveTo(baseX, h)
        for (y in h.toInt() downTo (h * 0.3).toInt() step 8) {
            val sway = sin(t * 0.8 + y * 0.01 + i * 1.5).toFloat() * swayAmplitude
            path.lineTo(baseX + sway, y.toFloat())
        }
        path.lineTo(baseX, (h * 0.3).toFloat())

        drawPath(path, kelpColor.copy(alpha = 0.4f), style = androidx.compose.ui.graphics.drawscope.Stroke(width = 6f))
    }
}

private fun DrawScope.drawCoralRidge(t: Double, w: Float, h: Float) {
    val path = Path()
    val baseY = h * 0.85f

    path.moveTo(0f, h)
    path.lineTo(0f, baseY)

    for (x in 0..w.toInt() step 4) {
        val xf = x.toFloat()
        val ridge = sin(xf * 0.02 + t * 0.3).toFloat() * 20f +
                    sin(xf * 0.05 + t * 0.1).toFloat() * 10f
        path.lineTo(xf, baseY + ridge)
    }

    path.lineTo(w, h)
    path.close()

    drawPath(path, Brush.verticalGradient(
        colors = listOf(CoralPink.copy(alpha = 0.3f), DeepWater),
        startY = baseY - 30f,
        endY = h
    ))
}

private fun DrawScope.drawBubbles(t: Double, w: Float, h: Float, voices: Int) {
    val bubbleCount = 5 + voices * 2
    val bubbleColor = BioGlow.copy(alpha = 0.15f)

    for (i in 0 until bubbleCount) {
        val seed = i * 137.5
        val x = ((seed * 7.3 + t * 20) % w.toDouble()).toFloat()
        val y = (h - ((seed * 3.1 + t * 40 + i * 100) % h.toDouble())).toFloat()
        val radius = 2f + (sin(seed + t) * 1.5).toFloat()

        drawCircle(bubbleColor, radius = radius, center = Offset(x, y))
    }
}

private fun DrawScope.drawBioGlow(t: Double, w: Float, h: Float, voices: Int) {
    if (voices == 0) return

    val glowColor = BioGlow.copy(alpha = 0.08f)
    for (v in 0 until minOf(voices, 8)) {
        val seed = v * 317.0
        val x = ((seed * 2.3) % w.toDouble()).toFloat()
        val y = h * 0.4f + ((seed * 1.7) % (h * 0.5).toDouble()).toFloat()
        val pulse = (sin(t * 1.5 + v * 0.9) + 1.0).toFloat() * 0.5f
        val radius = 30f + pulse * 40f

        drawCircle(glowColor, radius = radius, center = Offset(x, y))
    }
}
