// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
//==============================================================================
// OceanLayoutConstants.h — Single source of truth for OceanView/OceanLayout
// vertical layout budget. Both OceanView.h and OceanLayout.h alias these via
// `static constexpr int kFoo = ocean_layout::kFoo;` to preserve unqualified
// access at usage sites while collapsing the prior duplicate declarations.
//
// Window-height derivation:
//   minHeight     = kDashboardH + kStatusBarH + kWaterlineH + kMinOceanViewportH
//   defaultHeight = kDashboardH + kStatusBarH + kWaterlineH + kDefaultOceanViewportH
//
// Dashboard breakdown (kDashboardH = 504, KEYS-mode budget):
//   kMacroStripH (60) + kMasterFxStripH (48) + kEpicSlotsH (140)
//   + kTabBarH (30) + kSeqStripH (24) + kPlaySurfaceH (202) = 504
//
// SurfaceRight-open budget (PAD/XY mode, EpicSlots+SeqStrip hidden):
//   kMacroStripH (60) + kMasterFxStripH (48) + kTabBarH (30) = 138
//==============================================================================

namespace ocean_layout
{

static constexpr int   kStatusBarH             = 28;
static constexpr float kMacroStripH            = 60.0f;  // #901: 56→60pt to fit 48pt knobs + 6pt pad
static constexpr int   kWaterlineH             = 6;
static constexpr int   kDashboardH             = 504;   // I5a: bumped 340→504 for full dashboard
static constexpr int   kTabBarH                = 30;

// Min ocean viewport above which the dashboard layout no longer overflows.
// Sets the minimum window height: 504 + 28 + 6 + 226 = 764.
static constexpr int   kMinOceanViewportH      = 226;

// Default ocean viewport — gives generous breathing room above the dashboard.
// Sets the default window height: 504 + 28 + 6 + 326 = 864.
static constexpr int   kDefaultOceanViewportH  = 326;

} // namespace ocean_layout
