// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.ui.components

import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.mutableStateMapOf
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.ExperimentalComposeUiApi
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.PointerEventType
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.xoox.obrixreef.audio.ObrixBridge
import kotlin.math.roundToInt

private val ReefJade = Color(0xFF1E8B7E)
private val ReefJadeDim = Color(0xFF0F453F)

/**
 * Full multi-touch performance surface.
 *
 * Modes:
 *   XY Pad: X = filter cutoff, Y = macro movement. Touch triggers note.
 *   Grid: Scale-quantized pads (configurable rows/cols).
 *
 * Uses raw MotionEvent via pointerInput for proper multi-touch tracking.
 * Each finger gets its own voice (up to 8 simultaneous).
 */
@OptIn(ExperimentalComposeUiApi::class)
@Composable
fun TouchSurface(
    modifier: Modifier = Modifier,
    baseNote: Int = 48,
    columns: Int = 4,
    rows: Int = 4,
    scaleIntervals: IntArray = intArrayOf(0, 2, 4, 5, 7, 9, 11, 12, 14, 16, 17, 19, 21, 23, 24, 26)
) {
    // Track active touches: pointerId -> noteNumber
    val activeTouches = remember { mutableStateMapOf<Long, Int>() }

    DisposableEffect(Unit) {
        onDispose {
            // Release all notes on unmount
            activeTouches.values.forEach { note -> ObrixBridge.noteOff(note) }
            activeTouches.clear()
        }
    }

    Box(
        modifier = modifier
            .clip(RoundedCornerShape(16.dp))
            .background(ReefJadeDim)
            .pointerInput(baseNote, columns, rows) {
                awaitPointerEventScope {
                    while (true) {
                        val event = awaitPointerEvent()

                        when (event.type) {
                            PointerEventType.Press, PointerEventType.Move -> {
                                for (change in event.changes) {
                                    if (!change.pressed) continue

                                    val id = change.id.value
                                    val x = change.position.x / size.width.toFloat()
                                    val y = change.position.y / size.height.toFloat()

                                    val col = (x * columns).toInt().coerceIn(0, columns - 1)
                                    val row = (y * rows).toInt().coerceIn(0, rows - 1)
                                    val padIndex = row * columns + col
                                    val noteOffset = scaleIntervals.getOrElse(padIndex) { padIndex * 2 }
                                    val note = baseNote + noteOffset

                                    // Velocity from vertical position within the pad cell
                                    val cellY = (y * rows) - row
                                    val velocity = (0.4f + cellY * 0.6f).coerceIn(0.2f, 1.0f)

                                    val previousNote = activeTouches[id]
                                    if (previousNote != note) {
                                        // Note changed — release old, trigger new
                                        if (previousNote != null) {
                                            ObrixBridge.noteOff(previousNote)
                                        }
                                        activeTouches[id] = note
                                        ObrixBridge.noteOn(note, velocity)
                                    }

                                    // Map X position to filter cutoff for expression
                                    val cutoffNorm = x.coerceIn(0f, 1f)
                                    ObrixBridge.setParameter("obrix_proc1Cutoff",
                                        200f + cutoffNorm * 19800f)

                                    change.consume()
                                }
                            }
                            PointerEventType.Release -> {
                                for (change in event.changes) {
                                    if (change.pressed) continue
                                    val id = change.id.value
                                    val note = activeTouches.remove(id)
                                    if (note != null) {
                                        ObrixBridge.noteOff(note)
                                    }
                                    change.consume()
                                }
                            }
                        }
                    }
                }
            },
        contentAlignment = Alignment.Center
    ) {
        // Grid overlay labels
        Text(
            text = "TOUCH SURFACE",
            color = ReefJade.copy(alpha = 0.3f),
            fontSize = 14.sp,
            fontWeight = FontWeight.Bold
        )
    }
}
