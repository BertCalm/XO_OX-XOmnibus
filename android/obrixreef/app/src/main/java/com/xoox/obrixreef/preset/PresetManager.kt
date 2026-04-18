// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
package com.xoox.obrixreef.preset

import android.content.Context
import android.util.Log
import com.xoox.obrixreef.audio.ObrixBridge
import org.json.JSONObject

/**
 * Loads and applies .xometa preset files.
 * Reads from Android assets (bundled) and app-scoped storage (user).
 *
 * .xometa format: JSON with schema_version 1, matching desktop/iOS.
 */
class PresetManager(private val context: Context) {

    companion object {
        private const val TAG = "PresetManager"
        private const val ASSETS_DIR = "presets"
    }

    data class PresetInfo(
        val name: String,
        val mood: String,
        val author: String,
        val fileName: String,
        val tags: List<String> = emptyList()
    )

    private val presetCache = mutableListOf<PresetInfo>()

    /**
     * Scan bundled presets from assets/presets/ directory.
     */
    fun scanPresets(): List<PresetInfo> {
        presetCache.clear()

        try {
            val files = context.assets.list(ASSETS_DIR) ?: emptyArray()
            for (fileName in files) {
                if (!fileName.endsWith(".xometa")) continue

                try {
                    val json = context.assets.open("$ASSETS_DIR/$fileName")
                        .bufferedReader().use { it.readText() }
                    val obj = JSONObject(json)

                    presetCache.add(PresetInfo(
                        name = obj.optString("name", fileName.removeSuffix(".xometa")),
                        mood = obj.optString("mood", "Foundation"),
                        author = obj.optString("author", "XO_OX"),
                        fileName = fileName,
                        tags = obj.optJSONArray("tags")?.let { arr ->
                            (0 until arr.length()).map { arr.getString(it) }
                        } ?: emptyList()
                    ))
                } catch (e: Exception) {
                    Log.w(TAG, "Failed to parse $fileName: ${e.message}")
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Failed to scan presets: ${e.message}")
        }

        Log.i(TAG, "Scanned ${presetCache.size} presets")
        return presetCache.toList()
    }

    /**
     * Load and apply a preset by filename.
     */
    fun loadPreset(fileName: String): Boolean {
        try {
            val json = context.assets.open("$ASSETS_DIR/$fileName")
                .bufferedReader().use { it.readText() }
            return applyPresetJson(json)
        } catch (e: Exception) {
            Log.e(TAG, "Failed to load $fileName: ${e.message}")
            return false
        }
    }

    /**
     * Apply a parsed .xometa JSON string to the engine.
     * Extracts parameters from the "parameters" object and applies each one.
     */
    fun applyPresetJson(json: String): Boolean {
        try {
            val root = JSONObject(json)
            val engines = root.optJSONArray("engines")
            val params = root.optJSONObject("parameters") ?: return false

            // Find Obrix parameters (keyed by engine name)
            val obrixParams = params.optJSONObject("Obrix")
                ?: params.optJSONObject("obrix")

            if (obrixParams != null) {
                val keys = obrixParams.keys()
                var count = 0
                while (keys.hasNext()) {
                    val key = keys.next()
                    val value = obrixParams.optDouble(key, Double.NaN)
                    if (!value.isNaN()) {
                        ObrixBridge.setParameter(key, value.toFloat())
                        count++
                    }
                }
                Log.i(TAG, "Applied preset: ${root.optString("name")} ($count params)")
                return true
            }

            // Fallback: try flat parameter keys (some presets use "obrix_paramName" at root)
            val allKeys = params.keys()
            var count = 0
            while (allKeys.hasNext()) {
                val key = allKeys.next()
                if (key.startsWith("obrix_")) {
                    val value = params.optDouble(key, Double.NaN)
                    if (!value.isNaN()) {
                        ObrixBridge.setParameter(key, value.toFloat())
                        count++
                    }
                }
            }

            if (count > 0) {
                Log.i(TAG, "Applied preset (flat): ${root.optString("name")} ($count params)")
                return true
            }

            Log.w(TAG, "No Obrix parameters found in preset")
            return false
        } catch (e: Exception) {
            Log.e(TAG, "Failed to apply preset: ${e.message}")
            return false
        }
    }

    fun getPresets(): List<PresetInfo> = presetCache.toList()

    fun getPresetsByMood(mood: String): List<PresetInfo> =
        presetCache.filter { it.mood.equals(mood, ignoreCase = true) }
}
