// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.ui.screens

import androidx.compose.foundation.background
import androidx.compose.foundation.clickable
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.lazy.LazyColumn
import androidx.compose.foundation.lazy.items
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.FilterChip
import androidx.compose.material3.FilterChipDefaults
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateListOf
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.Modifier
import androidx.compose.ui.draw.clip
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.compose.ui.unit.sp
import com.xoox.obrixreef.preset.PresetManager

private val ReefJade = Color(0xFF1E8B7E)
private val DarkBackground = Color(0xFF0A0A0A)
private val SurfaceColor = Color(0xFF1A1A1A)
private val CardColor = Color(0xFF222222)

// XOceanus mood categories
private val moods = listOf(
    "All", "Foundation", "Atmosphere", "Entangled", "Prism",
    "Flux", "Aether", "Submerged", "Coupling",
    "Crystalline", "Deep", "Ethereal", "Kinetic",
    "Luminous", "Organic", "Shadow"
)

@Composable
fun PresetBrowserScreen(
    presetManager: PresetManager,
    onPresetSelected: (String) -> Unit,
    onBack: () -> Unit
) {
    val presets = remember { mutableStateListOf<PresetManager.PresetInfo>() }
    var selectedMood by remember { mutableStateOf("All") }

    LaunchedEffect(Unit) {
        presets.clear()
        presets.addAll(presetManager.scanPresets())
    }

    val filtered = if (selectedMood == "All") presets
                   else presets.filter { it.mood.equals(selectedMood, ignoreCase = true) }

    Column(
        modifier = Modifier
            .fillMaxSize()
            .background(DarkBackground)
            .padding(16.dp),
        verticalArrangement = Arrangement.spacedBy(12.dp)
    ) {
        // Header
        Row(
            modifier = Modifier.fillMaxWidth().padding(top = 48.dp),
            horizontalArrangement = Arrangement.SpaceBetween
        ) {
            Text(
                text = "PRESETS",
                color = ReefJade,
                fontSize = 24.sp,
                fontWeight = FontWeight.Bold
            )
            Text(
                text = "BACK",
                color = ReefJade,
                fontSize = 14.sp,
                modifier = Modifier.clickable { onBack() }.padding(8.dp)
            )
        }

        Text(
            text = "${filtered.size} presets",
            color = Color.Gray,
            fontSize = 12.sp
        )

        // Mood filter chips (horizontal scroll)
        Row(
            modifier = Modifier.fillMaxWidth(),
            horizontalArrangement = Arrangement.spacedBy(6.dp)
        ) {
            for (mood in moods.take(6)) {
                FilterChip(
                    selected = selectedMood == mood,
                    onClick = { selectedMood = mood },
                    label = { Text(mood, fontSize = 11.sp) },
                    colors = FilterChipDefaults.filterChipColors(
                        selectedContainerColor = ReefJade,
                        selectedLabelColor = Color.White,
                        containerColor = SurfaceColor,
                        labelColor = Color.Gray
                    )
                )
            }
        }

        // Preset list
        LazyColumn(
            modifier = Modifier.weight(1f),
            verticalArrangement = Arrangement.spacedBy(4.dp)
        ) {
            items(filtered) { preset ->
                PresetCard(preset) {
                    onPresetSelected(preset.fileName)
                }
            }
        }
    }
}

@Composable
fun PresetCard(preset: PresetManager.PresetInfo, onClick: () -> Unit) {
    Row(
        modifier = Modifier
            .fillMaxWidth()
            .clip(RoundedCornerShape(8.dp))
            .background(CardColor)
            .clickable { onClick() }
            .padding(12.dp),
        horizontalArrangement = Arrangement.SpaceBetween
    ) {
        Column {
            Text(
                text = preset.name,
                color = Color.White,
                fontSize = 14.sp,
                fontWeight = FontWeight.Medium
            )
            Text(
                text = "${preset.mood} · ${preset.author}",
                color = Color.Gray,
                fontSize = 11.sp
            )
        }
    }
}
