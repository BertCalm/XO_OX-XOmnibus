// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include "XOutshine.h"

//==============================================================================
//  XOriginate — The Sample Import Wizard
//  "I have samples. Make them into instruments."
//
//  EXPORT PYRAMID ARCHITECTURE:
//
//    ORIGINATE  ← user-facing UI wizard (this layer)
//        │         "import samples, choose profile, preview, export"
//        │
//    OUTSHINE   ← DSP engine (the processing brain)
//        │         classify → enhance → layer → normalize → map
//        │
//    OXPORT     ← batch CLI pipeline (the factory)
//                  render → QA → trim → export → cover art → package
//                  calls Outshine internally for sample processing
//
//  Originate IS the front door. It guides producers through:
//    1. Drag & drop samples into the import zone
//    2. Choose a Rebirth profile (or auto-detect from sample character)
//    3. Preview the enhanced result in real-time
//    4. Export as XPN instrument or WAV folder
//
//  Under the hood, Originate delegates all DSP to XOutshine.
//  For batch/automated workflows (expansion packs at scale),
//  use Oxport (Tools/oxport.py) instead.
//
//  See: Docs/export-architecture.md
//       Source/UI/Outshine/OutshineMainComponent.h  (Originate UI)
//       Source/Export/XOutshine.h                   (DSP engine)
//       Tools/oxport.py                             (batch CLI)
//==============================================================================

namespace xo {

//------------------------------------------------------------------------------
// OriginateConfig — parameters passed from the UI wizard to the DSP pipeline.
// Populated by OutshineMainComponent and forwarded to XOutshine::process().
//------------------------------------------------------------------------------

struct OriginateConfig
{
    juce::StringArray samplePaths;      // Input sample files (absolute paths)
    juce::String      rebirthProfile;   // "auto", "obrix", "onset", "oware", "opera", "overwash"
    juce::String      outputFormat;     // "xpn" (MPC instrument) or "folder" (WAV tree)
    juce::File        outputDir;
    bool autoDetectProfile = true;      // When true, ignores rebirthProfile and classifies samples
    bool generatePreview   = true;      // Render a 4-bar preview loop before final export
};

// Originate orchestrates the UI wizard flow.
// For the actual DSP, see XOutshine.h (xoceanus::XOutshineEngine).
// For batch CLI export, see Tools/oxport.py.

} // namespace xo
