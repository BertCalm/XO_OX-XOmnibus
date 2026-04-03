/*
    XOceanus Test Runner — Catch2 v3
    =================================
    Main entry point that registers all test suites under Catch2.
    Catch2WithMain provides its own main(); this file just pulls in
    the individual translation units via their headers.

    Migration: custom runner → Catch2 v3 (issue #81)
*/

// Catch2 main is provided by the Catch2::Catch2WithMain link target.
// All test logic lives in the per-suite .cpp files; including the headers
// here triggers their Catch2 TEST_CASE registrations when the linker
// merges the translation units.

#include "DSPTests/DSPStabilityTests.h"
#include "DSPTests/DSPComponentTests.h"
#include "CouplingTests/CouplingMatrixTests.h"
#include "PresetTests/PresetRoundTripTests.h"
#include "PresetTests/BackwardCompatibilityTests.h"
#include "ExportTests/XPNExportTests.h"
#include "DoctrineTests/DoctrineTests.h"
#include "PlaySurfaceTests/HarmonicFieldTests.h"
#include "PlaySurfaceTests/GestureTrailTests.h"
#include "PipelineTests/FullPipelineTests.h"
#include "ParameterSweepTests/ParameterSweepTests.h"
#include "BenchmarkTests/CPUBenchmarkTests.h"
#include "RegressionTests/AudioRegressionTests.h"

// No main() here — Catch2WithMain supplies it.
