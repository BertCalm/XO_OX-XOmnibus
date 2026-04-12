// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.height
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.size
import androidx.compose.material3.Slider
import androidx.compose.material3.SliderDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableFloatStateOf
import androidx.compose.runtime.mutableIntStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.xoox.obrixreef.audio.ObrixBridge
import com.xoox.obrixreef.ui.components.ReefCanvas
import com.xoox.obrixreef.ui.components.TouchSurface
import kotlinx.coroutines.delay

private val ReefJade = Color(0xFF1E8B7E)
private val ReefJadeDim = Color(0xFF0F453F)
private val DarkBackground = Color(0xFF0A0A0A)

@Composable
fun PlayScreen(
    onNavigateToPresets: () -> Unit
) {
    var activeVoices by remember { mutableIntStateOf(0) }

    // Poll voice count for display
    LaunchedEffect(Unit) {
        while (true) {
            activeVoices = ObrixBridge.activeVoiceCount
            delay(100)
        }
    }

    Box(modifier = Modifier.fillMaxSize().background(DarkBackground)) {
        // Background reef animation
        ReefCanvas(
            modifier = Modifier.fillMaxSize(),
            activeVoices = activeVoices
        )

        // Foreground UI
        Column(
            modifier = Modifier
                .fillMaxSize()
                .padding(16.dp),
            verticalArrangement = Arrangement.spacedBy(12.dp)
        ) {
            // Header
            Row(
                modifier = Modifier.fillMaxWidth().padding(top = 48.dp),
                horizontalArrangement = Arrangement.SpaceBetween,
                verticalAlignment = Alignment.CenterVertically
            ) {
                Column {
                    Text(
                        text = "OBRIX REEF",
                        color = ReefJade,
                        fontSize = 24.sp,
                        fontWeight = FontWeight.Bold
                    )
                    Text(
                        text = "The Living Reef",
                        color = Color.Gray,
                        fontSize = 12.sp
                    )
                }

                Text(
                    text = "PRESETS",
                    color = ReefJade,
                    fontSize = 14.sp,
                    fontWeight = FontWeight.Medium,
                    modifier = Modifier
                        .clickable { onNavigateToPresets() }
                        .padding(8.dp)
                )
            }

            // Multi-touch performance surface
            TouchSurface(
                modifier = Modifier
                    .fillMaxWidth()
                    .weight(1f),
                baseNote = 48,
                columns = 4,
                rows = 4
            )

            // Macro sliders
            MacroSection()

            // Status bar
            Row(
                modifier = Modifier
                    .fillMaxWidth()
                    .padding(bottom = 24.dp),
                horizontalArrangement = Arrangement.SpaceBetween
            ) {
                Text(
                    text = "Voices: $activeVoices",
                    color = Color.Gray,
                    fontSize = 11.sp
                )
                Text(
                    text = "${ObrixBridge.sampleRate.toInt()} Hz",
                    color = Color.Gray,
                    fontSize = 11.sp
                )
            }
        }
    }
}

@Composable
private fun MacroSection() {
    val macros = listOf(
        "CHARACTER" to "obrix_macroCharacter",
        "MOVEMENT" to "obrix_macroMovement",
        "COUPLING" to "obrix_macroCoupling",
        "SPACE" to "obrix_macroSpace"
    )

    Column(verticalArrangement = Arrangement.spacedBy(2.dp)) {
        Text(
            text = "MACROS",
            color = ReefJade,
            fontSize = 11.sp,
            fontWeight = FontWeight.Bold
        )

        macros.forEach { (name, paramId) ->
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
                    modifier = Modifier.size(80.dp, 18.dp)
                )

                Slider(
                    value = value,
                    onValueChange = {
                        value = it
                        ObrixBridge.setParameter(paramId, it)
                    },
                    modifier = Modifier.weight(1f).height(20.dp),
                    colors = SliderDefaults.colors(
                        thumbColor = ReefJade,
                        activeTrackColor = ReefJade,
                        inactiveTrackColor = ReefJadeDim
                    )
                )
            }
        }
    }
}
