// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef

import android.os.Bundle
import android.util.Log
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.compose.material3.MaterialTheme
import androidx.compose.runtime.getValue
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.remember
import androidx.compose.runtime.setValue
import androidx.compose.ui.graphics.Color
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.rememberNavController
import com.xoox.obrixreef.audio.MidiInputHandler
import com.xoox.obrixreef.audio.ObrixBridge
import com.xoox.obrixreef.glyph.FaceDownDetector
import com.xoox.obrixreef.glyph.GlyphReefService
import com.xoox.obrixreef.preset.PresetManager
import com.xoox.obrixreef.ui.screens.PlayScreen
import com.xoox.obrixreef.ui.screens.PresetBrowserScreen

private val ReefJade = Color(0xFF1E8B7E)
private val DarkBackground = Color(0xFF0A0A0A)
private val SurfaceColor = Color(0xFF1A1A1A)

class MainActivity : ComponentActivity() {

    private var midiHandler: MidiInputHandler? = null
    private var glyphService: GlyphReefService? = null
    private var faceDownDetector: FaceDownDetector? = null
    private lateinit var presetManager: PresetManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()

        presetManager = PresetManager(this)

        // Face-down detection -> Reef Aquarium mode
        faceDownDetector = FaceDownDetector(this).apply {
            listener = object : FaceDownDetector.Listener {
                override fun onFaceDownChanged(isFaceDown: Boolean) {
                    if (isFaceDown) {
                        Log.i("ObrixReef", "Reef Aquarium mode: ON")
                        // Activate Journey Mode (autonomous drift)
                        ObrixBridge.setParameter("obrix_journeyMode", 1.0f)
                        ObrixBridge.setParameter("obrix_driftDepth", 0.4f)
                        // Start Glyph visualization
                        glyphService?.start()
                    } else {
                        Log.i("ObrixReef", "Reef Aquarium mode: OFF")
                        ObrixBridge.setParameter("obrix_journeyMode", 0.0f)
                        ObrixBridge.setParameter("obrix_driftDepth", 0.0f)
                        glyphService?.stop()
                    }
                }
            }
        }

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
                val navController = rememberNavController()

                NavHost(navController = navController, startDestination = "play") {
                    composable("play") {
                        PlayScreen(
                            onNavigateToPresets = { navController.navigate("presets") }
                        )
                    }
                    composable("presets") {
                        PresetBrowserScreen(
                            presetManager = presetManager,
                            onPresetSelected = { fileName ->
                                presetManager.loadPreset(fileName)
                                navController.popBackStack()
                            },
                            onBack = { navController.popBackStack() }
                        )
                    }
                }
            }
        }
    }

    override fun onResume() {
        super.onResume()
        ObrixBridge.start()
        midiHandler = MidiInputHandler(this).also { it.start() }
        faceDownDetector?.start()
        glyphService = GlyphReefService(this)
        Log.i("ObrixReef", "All systems active")
    }

    override fun onPause() {
        super.onPause()
        glyphService?.stop()
        faceDownDetector?.stop()
        midiHandler?.stop()
        midiHandler = null
        ObrixBridge.allNotesOff()
        ObrixBridge.stop()
        Log.i("ObrixReef", "All systems paused")
    }
}
