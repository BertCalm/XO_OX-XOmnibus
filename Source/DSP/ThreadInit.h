#pragma once

//==============================================================================
// ThreadInit.h — Audio thread denormal prevention via CPU control registers.
//
// XOlokun CLAUDE.md: "Denormal protection required in all feedback/filter paths"
//
// WHY THIS EXISTS
// ---------------
// IEEE 754 denormal ("subnormal") numbers are 100× more expensive than normal
// floats on most CPUs because they fall through to microcode assist. IIR filters,
// feedback delay lines, and the 73-engine coupling matrix all accumulate denormals
// in their state variables. FastMath.h provides per-sample flushDenormal() as a
// belt-and-suspenders guard on individual state variables, but the cheapest fix is
// to tell the CPU to flush them at the hardware level before they ever reach user
// code.
//
// PLATFORM BEHAVIOUR
// ------------------
// ARM64 (Apple Silicon / iOS):
//   FPCR.FZ (bit 24, 0x01000000) — Flush-to-Zero mode. When set, any denormal
//   result is silently flushed to ±0 before being written to a register.
//   NOTE: juce::ScopedNoDenormals does NOT set FPCR.FZ on ARM64 as of JUCE 8.
//         It guards the MXCSR on x86 but is a no-op on ARM. This utility fills
//         that gap.
//
// x86 / x86-64:
//   MXCSR.DAZ (bit 6,  0x0040) — Denormals-Are-Zero (treat denormal *inputs* as 0)
//   MXCSR.FTZ (bit 15, 0x8000) — Flush-to-Zero (flush denormal *results* to 0)
//   Combined mask: 0x8040. juce::ScopedNoDenormals already does this; calling
//   initAudioThread() on x86 is still safe (idempotent OR-mask) and ensures DSP
//   threads that don't go through processBlock are also protected.
//
// WHERE TO CALL
// -------------
// In XOlokunProcessor::processBlock (Source/XOlokunProcessor.cpp, line ~1173):
//
//   void XOlokunProcessor::processBlock(juce::AudioBuffer<float>& buffer,
//                                       juce::MidiBuffer& midi)
//   {
//       xolokun::dsp::initAudioThreadOnce();   // <-- ADD THIS (first line of body)
//       juce::ScopedNoDenormals noDenormals;   // keep for x86 scoped guard
//       ...
//   }
//
// Also call initAudioThread() at the top of any offline/bounce/render thread
// that invokes engine DSP directly (e.g. XPN export worker threads).
//
// RELATIONSHIP TO FastMath.h
// --------------------------
// xolokun::flushDenormal(float) in FastMath.h provides per-sample bitcast
// flushing on individual state variables. That guard can remain active in
// Debug builds (#ifndef NDEBUG) as belt-and-suspenders validation. In Release
// builds, FPCR.FZ / MXCSR DAZ+FTZ make the per-sample call a no-op because
// denormals never survive to user code — but keeping it does not hurt correctness.
//==============================================================================

#include <mutex>

// SSE intrinsics for x86 MXCSR access
#if defined(__SSE__) || defined(_M_IX86) || defined(_M_X64) || defined(__x86_64__)
    #include <immintrin.h>
#endif

namespace xolokun {
namespace dsp {

//==============================================================================
/// initAudioThread() — set hardware flush-to-zero flags on the calling thread.
///
/// Safe to call multiple times on the same thread: both the ARM64 OR-mask and
/// the x86 OR-mask are idempotent (setting an already-set bit is a no-op).
///
/// Must be called once per DSP thread:
///   - Real-time audio callback thread (processBlock)
///   - Offline render / bounce worker threads
///   - Export worker threads (XPN pipeline)
///
/// Not required on the UI/message thread (no floating-point DSP runs there).
static inline void initAudioThread() noexcept
{
#if defined(__aarch64__) || defined(_M_ARM64)
    // ARM64: read FPCR, set FZ bit (bit 24), write back.
    // __builtin_arm_rsr64 / __builtin_arm_wsr64 are supported on:
    //   - Clang 3.8+ (Apple Clang, LLVM)
    //   - GCC 6+
    //   - MSVC ARM64 (via __arm64_sysreg_* intrinsics — not needed for Apple targets)
    //
    // FZ bit position: ARM DDI 0487 — FPCR bit [24] = FZ (Flush-to-Zero).
    constexpr uint64_t kFPCR_FZ = UINT64_C(0x01000000);
    const uint64_t fpcr = __builtin_arm_rsr64("fpcr");
    if ((fpcr & kFPCR_FZ) == 0)                 // skip the write if already set
        __builtin_arm_wsr64("fpcr", fpcr | kFPCR_FZ);

#elif defined(__SSE__) || defined(_M_IX86) || defined(_M_X64) || defined(__x86_64__)
    // x86/x86-64: set DAZ (bit 6 = 0x0040) + FTZ (bit 15 = 0x8000) in MXCSR.
    // DAZ treats denormal *inputs* as zero; FTZ flushes denormal *results* to zero.
    // Both are needed for complete protection of IIR feedback loops.
    //
    // Note: juce::ScopedNoDenormals already does this in processBlock, but
    // setting it here ensures the flags survive for the full thread lifetime
    // rather than only within the RAII scope — and protects threads that do
    // not go through processBlock (export workers, offline render, etc.).
    constexpr unsigned int kMXCSR_DAZ_FTZ = 0x8040u;
    _mm_setcsr(_mm_getcsr() | kMXCSR_DAZ_FTZ);

#else
    // Unsupported platform (e.g. WASM, unknown architecture) — no-op.
    // The per-sample xolokun::flushDenormal() in FastMath.h remains the
    // only denormal guard on these platforms.
    (void)0;
#endif
}

//==============================================================================
/// ScopedAudioThreadInit — RAII guard for DSP thread initialization.
///
/// Calls initAudioThread() in the constructor. The destructor is intentionally
/// empty: FTZ/DAZ flags should remain set for the entire thread lifetime.
/// Destroying this guard does NOT restore the previous MXCSR / FPCR state.
///
/// Usage:
///   // At the top of a worker thread entry point:
///   xolokun::dsp::ScopedAudioThreadInit audioInit;
///
/// For processBlock specifically, prefer initAudioThreadOnce() to avoid the
/// (trivial) overhead of the flag read on every block.
struct ScopedAudioThreadInit
{
    ScopedAudioThreadInit() noexcept  { initAudioThread(); }
    ~ScopedAudioThreadInit() noexcept { /* intentionally does not restore flags */ }

    // Non-copyable — this is a one-shot thread-state mutator.
    ScopedAudioThreadInit(const ScopedAudioThreadInit&)            = delete;
    ScopedAudioThreadInit& operator=(const ScopedAudioThreadInit&) = delete;
};

//==============================================================================
/// initAudioThreadOnce() — thread-safe one-shot initialization via std::call_once.
///
/// Intended for use at the very top of processBlock. The first call from any
/// thread initializes that thread's FPU flags; subsequent calls within the same
/// translation unit are a near-zero-cost check of the once_flag.
///
/// IMPORTANT: std::once_flag is per-translation-unit static — this ensures the
/// init runs once per *process*, not once per thread. For multi-threaded offline
/// render scenarios where separate worker threads need their own FPCR set,
/// call initAudioThread() directly at thread entry (e.g. via ScopedAudioThreadInit).
///
/// Usage in XOlokunProcessor.cpp:
///
///   void XOlokunProcessor::processBlock(juce::AudioBuffer<float>& buffer,
///                                       juce::MidiBuffer& midi)
///   {
///       xolokun::dsp::initAudioThreadOnce(); // ARM64 FPCR.FZ + x86 MXCSR DAZ+FTZ
///       juce::ScopedNoDenormals noDenormals; // x86 scoped guard (belt-and-suspenders)
///       ...
///   }
inline void initAudioThreadOnce() noexcept
{
    static std::once_flag sInitFlag;
    std::call_once(sInitFlag, []() noexcept { initAudioThread(); });
}

} // namespace dsp
} // namespace xolokun
