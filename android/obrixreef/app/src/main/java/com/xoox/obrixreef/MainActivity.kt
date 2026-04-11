// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef

import android.os.Bundle
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.foundation.background
import androidx.compose.foundation.gestures.detectTapGestures
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Surface
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.DisposableEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.input.pointer.pointerInput
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.xoox.obrixreef.audio.MidiInputHandler
import com.xoox.obrixreef.audio.ObrixBridge

// Obrix reef jade
val ReefJade = Color(0xFF1E8B7E)
val ReefJadeDim = Color(0xFF0F453F)
val DarkBackground = Color(0xFF0A0A0A)
val SurfaceColor = Color(0xFF1A1A1A)

class MainActivity : ComponentActivity() {

    private var midiHandler: MidiInputHandler? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        setContent {
            MaterialTheme(
                colorScheme = MaterialTheme.colorScheme.copy(
                    background = DarkBackground,
                    surface = SurfaceColor,
                    primary = ReefJade,
                    onBackground = Color.White,
                    onSurface = Color.White
                )
            ) {
                ObrixReefScreen()
            }
        }
    }

    override fun onResume() {
        super.onResume()
        ObrixBridge.start()
        midiHandler = MidiInputHandler(this).also { it.start() }
    }

    override fun onPause() {
        super.onPause()
        midiHandler?.stop()
        midiHandler = null
        ObrixBridge.allNotesOff()
        ObrixBridge.stop()
    }
}

@Composable
fun ObrixReefScreen() {
    DisposableEffect(Unit) {
        onDispose { ObrixBridge.allNotesOff() }
    }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(DarkBackground)
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(16.dp)
    ) {
        // Header
        Text(
            text = "OBRIX REEF",
            color = ReefJade,
            fontSize = 28.sp,
            fontWeight = FontWeight.Bold,
            modifier = Modifier.padding(top = 48.dp)
        )

        Text(
            text = "The Living Reef — Android Preview",
            color = Color.Gray,
            fontSize = 14.sp
        )

        // Touch Pads (2x4 grid of playable pads)
        TouchPadGrid(
            modifier = Modifier
                .fillMaxWidth()
                .weight(1f)
        )

        // Macro Knobs
        MacroSliders(
            modifier = Modifier.fillMaxWidth()
        )

        // Status bar
        var voiceCount by remember { mutableIntStateOf(0) }
        Text(
            text = "Voices: $voiceCount | ${ObrixBridge.sampleRate.toInt()} Hz",
            color = Color.Gray,
            fontSize = 12.sp,
            modifier = Modifier.padding(bottom = 24.dp)
        )
    }
}

@Composable
fun TouchPadGrid(modifier: Modifier = Modifier) {
    val notes = listOf(60, 62, 64, 65, 67, 69, 71, 72) // C major scale
    val noteNames = listOf("C4", "D4", "E4", "F4", "G4", "A4", "B4", "C5")

    Column(
        modifier = modifier,
        verticalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        for (row in 0..1) {
            Row(
                modifier = Modifier.fillMaxWidth().weight(1f),
                horizontalArrangement = Arrangement.spacedBy(8.dp)
            ) {
                for (col in 0..3) {
                    val idx = row * 4 + col
                    TouchPad(
                        note = notes[idx],
                        label = noteNames[idx],
                        modifier = Modifier.weight(1f).fillMaxSize()
                    )
                }
            }
        }
    }
}

@Composable
fun TouchPad(note: Int, label: String, modifier: Modifier = Modifier) {
    var isPressed by remember { mutableFloatStateOf(0f) }
    val bgColor = if (isPressed > 0f) ReefJade else ReefJadeDim

    Box(
        modifier = modifier
            .clip(RoundedCornerShape(12.dp))
            .background(bgColor)
            .pointerInput(note) {
                detectTapGestures(
                    onPress = {
                        isPressed = 0.8f
                        ObrixBridge.noteOn(note, 0.8f)
                        tryAwaitRelease()
                        isPressed = 0f
                        ObrixBridge.noteOff(note)
                    }
                )
            },
        contentAlignment = Alignment.Center
    ) {
        Text(
            text = label,
            color = Color.White,
            fontSize = 16.sp,
            fontWeight = FontWeight.Medium
        )
    }
}

@Composable
fun MacroSliders(modifier: Modifier = Modifier) {
    val macros = listOf(
        "CHARACTER" to "obrix_macroCharacter",
        "MOVEMENT" to "obrix_macroMovement",
        "COUPLING" to "obrix_macroCoupling",
        "SPACE" to "obrix_macroSpace"
    )

    Column(modifier = modifier, verticalArrangement = Arrangement.spacedBy(4.dp)) {
        Text(
            text = "MACROS",
            color = ReefJade,
            fontSize = 12.sp,
            fontWeight = FontWeight.Bold
        )

        macros.forEach { (name, paramId) ->
            MacroSlider(name = name, paramId = paramId)
        }
    }
}

@Composable
fun MacroSlider(name: String, paramId: String) {
    var value by remember { mutableFloatStateOf(0f) }

    Row(
        modifier = Modifier.fillMaxWidth(),
        verticalAlignment = Alignment.CenterVertically,
        horizontalArrangement = Arrangement.spacedBy(8.dp)
    ) {
        Text(
            text = name,
            color = Color.Gray,
            fontSize = 10.sp,
            modifier = Modifier.size(80.dp, 20.dp)
        )

        Slider(
            value = value,
            onValueChange = {
                value = it
                ObrixBridge.setParameter(paramId, it)
            },
            modifier = Modifier.weight(1f).height(24.dp),
            colors = SliderDefaults.colors(
                thumbColor = ReefJade,
                activeTrackColor = ReefJade,
                inactiveTrackColor = ReefJadeDim
            )
        )
    }
}
