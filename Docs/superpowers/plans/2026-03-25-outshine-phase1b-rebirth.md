# Phase 1B: Outshine Rebirth Mode Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task.

**Goal:** Implement engine-inspired FX chain Rebirth Mode for Outshine — 5 profiles that transform producer samples through DSP chains encoding each XOlokun engine's sonic character.

**Architecture:** 3 new header files (RebirthDSP.h, RebirthProfiles.h, RebirthPipeline.h) in Source/Export/. Pipeline: resample → LUFS normalize → audio analysis → FX chain (per-profile) → gain compensate → true-peak limit. Wraps existing DSP modules from Source/DSP/Effects/. No SynthEngine instantiation — DSP chains inspired by engines, not live engine routing.

**Tech Stack:** C++17, JUCE 8, CMake + Ninja, inline .h headers

**Spec:** `Docs/superpowers/specs/2026-03-25-outshine-phase1b-rebirth-design.md`

---

### Task 1: RebirthDSP.h — New DSP Modules

**What:** Create `Source/Export/RebirthDSP.h` containing four new DSP modules: AllpassDiffuser, FormantResonator, NoiseBurst, and the inline SoftClipGuard function. These modules have no dependency on other new Rebirth files.

**Why:** All other Rebirth files depend on the module types and algorithms defined here. Must be written first. Wraps existing modules where possible (Saturator, Combulator, etc. are not reimplemented here — this file adds only algorithms that do not already exist in `Source/DSP/Effects/`).

**Files:**
- Create: `Source/Export/RebirthDSP.h`

---

- [ ] **Step 1.1 — Scaffold the file and SoftClipGuard**

  Create `Source/Export/RebirthDSP.h` with the file header, includes, namespace, and the inline `softClip` guard function.

  ```cpp
  #pragma once
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>
  #include "../DSP/FastMath.h"

  #include <array>
  #include <cmath>
  #include <complex>
  #include <vector>

  namespace xolokun {

  //==============================================================================
  // SoftClipGuard — Inline inter-stage headroom limiter.
  // Soft-limits signal to ±1.0 using tanh shape. Gain-neutral at low levels
  // (tanh(0.7)/tanh(0.7) = 1.0 at the normalization point).
  // Insert between any two stages that add gain to prevent hard clipping.
  //==============================================================================
  inline float softClip (float x) noexcept
  {
      constexpr float kDrive = 0.7f;
      constexpr float kNorm  = 0.6043677f; // 1.0f / tanh(0.7f)
      return std::tanh (x * kDrive) * kNorm;
  }

  inline void softClipBlock (float* data, int numSamples) noexcept
  {
      for (int i = 0; i < numSamples; ++i)
          data[i] = softClip (data[i]);
  }
  ```

  Verify: the file compiles as a header-only unit with a trivial `.cpp` stub (not required for the build, just mental check).

  Commit: `Add RebirthDSP.h scaffold with SoftClipGuard inline functions`

---

- [ ] **Step 1.2 — AllpassDiffuser**

  Append the `AllpassDiffuser` class to `RebirthDSP.h` inside the `xolokun` namespace, before the closing `}`.

  ```cpp
  //==============================================================================
  // AllpassDiffuser — 4-stage cascaded allpass filter for temporal diffusion.
  //
  // Each stage implements: y[n] = -g*x[n] + x[n-D] + g*y[n-D]
  // Delay times are set in milliseconds; feedback per stage is configurable.
  // Prime-ish delay ratios (7.1, 11.3, 17.9, 23.7 ms) avoid spectral coloration.
  //
  // OVERWASH profile uses this for "deep water" diffusion of transient content.
  //==============================================================================
  class AllpassDiffuser
  {
  public:
      static constexpr int kNumStages = 4;

      // Default delay times (ms) — prime-ish ratios avoid comb resonance artifacts
      static constexpr float kDefaultDelayMs[kNumStages] = { 7.1f, 11.3f, 17.9f, 23.7f };

      AllpassDiffuser() = default;

      void prepare (double sampleRate, int /*maxBlockSize*/)
      {
          sr_ = sampleRate;
          for (int s = 0; s < kNumStages; ++s)
          {
              delayMs_[s]   = kDefaultDelayMs[s];
              feedback_[s]  = 0.6f;
          }
          allocateBuffers();
          reset();
      }

      void setDelayMs (int stage, float ms)
      {
          jassert (stage >= 0 && stage < kNumStages);
          delayMs_[stage] = juce::jlimit (0.5f, 500.0f, ms);
          allocateBuffers();
      }

      void setFeedback (int stage, float g)
      {
          jassert (stage >= 0 && stage < kNumStages);
          feedback_[stage] = juce::jlimit (-0.99f, 0.99f, g);
      }

      // Set all-stage feedback uniformly (convenience for profile parameter application)
      void setFeedbackAll (float g)
      {
          for (int s = 0; s < kNumStages; ++s)
              feedback_[s] = juce::jlimit (-0.99f, 0.99f, g);
      }

      void reset()
      {
          for (int s = 0; s < kNumStages; ++s)
          {
              if (!bufL_[s].empty()) std::fill (bufL_[s].begin(), bufL_[s].end(), 0.0f);
              if (!bufR_[s].empty()) std::fill (bufR_[s].begin(), bufR_[s].end(), 0.0f);
              posL_[s] = posR_[s] = 0;
          }
      }

      // Process a stereo block in-place.
      void processBlock (float* left, float* right, int numSamples)
      {
          for (int i = 0; i < numSamples; ++i)
          {
              float xL = left[i];
              float xR = right[i];

              for (int s = 0; s < kNumStages; ++s)
              {
                  int delaySamps = delaySamples_[s];
                  auto& bL = bufL_[s];
                  auto& bR = bufR_[s];
                  int& pL = posL_[s];
                  int& pR = posR_[s];
                  float g = feedback_[s];

                  // Allpass: y[n] = -g*x[n] + x[n-D] + g*y[n-D]
                  // The delay line stores y[n], so x[n-D] is retrieved from D samples ago
                  // and g*y[n-D] is already embedded in the delay line content.
                  // Schroeder allpass implementation:
                  //   delayed = buf[pos]         (this is x[n-D] in the input delay view,
                  //                               but we store the mixed signal)
                  // Standard implementation: store input, read delayed, compute output, write output.
                  // Use dual-buffer: inputBuf for x[n-D], outputBuf for y[n-D].
                  // For simplicity and correctness use the one-buffer form:
                  //   v     = x - g * buf[pos]
                  //   y     = buf[pos] + g * v
                  //   buf[pos] = v
                  float vL = xL - g * bL[pL];
                  float yL = bL[pL] + g * vL;
                  bL[pL]   = vL;
                  pL = (pL + 1) % delaySamps;

                  float vR = xR - g * bR[pR];
                  float yR = bR[pR] + g * vR;
                  bR[pR]   = vR;
                  pR = (pR + 1) % delaySamps;

                  // Denormal protection
                  yL += 1e-25f; yL -= 1e-25f;
                  yR += 1e-25f; yR -= 1e-25f;

                  xL = yL;
                  xR = yR;
              }

              left[i]  = xL;
              right[i] = xR;
          }
      }

  private:
      void allocateBuffers()
      {
          for (int s = 0; s < kNumStages; ++s)
          {
              int samps = juce::jmax (1, (int) std::ceil (delayMs_[s] * 0.001 * sr_)) + 2;
              delaySamples_[s] = samps;
              bufL_[s].assign ((size_t) samps, 0.0f);
              bufR_[s].assign ((size_t) samps, 0.0f);
          }
      }

      double sr_ = 44100.0;
      float  delayMs_[kNumStages]    = { 7.1f, 11.3f, 17.9f, 23.7f };
      float  feedback_[kNumStages]   = { 0.6f, 0.6f, 0.6f, 0.6f };
      int    delaySamples_[kNumStages] = { 0, 0, 0, 0 };

      std::vector<float> bufL_[kNumStages];
      std::vector<float> bufR_[kNumStages];
      int posL_[kNumStages] = {};
      int posR_[kNumStages] = {};
  };
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add AllpassDiffuser to RebirthDSP.h — 4-stage Schroeder allpass cascade`

---

- [ ] **Step 1.3 — NoiseBurst**

  Append `NoiseBurst` to `RebirthDSP.h` inside the namespace.

  ```cpp
  //==============================================================================
  // NoiseBurst — Short filtered white-noise transient injector.
  //
  // Generates a brief burst of HP-filtered white noise mixed into the signal.
  // Fired once at block start if triggerBurst() was called. The burst length,
  // HP filter cutoff, and level are all configurable.
  //
  // ONSET profile uses this to add "crack" to the attack of percussive material.
  //==============================================================================
  class NoiseBurst
  {
  public:
      NoiseBurst() = default;

      void prepare (double sampleRate, int /*maxBlockSize*/)
      {
          sr_        = sampleRate;
          burstPhase_ = 0;
          burstActive_ = false;
          updateHPF();
          reset();
      }

      /// Length of the noise burst in milliseconds.
      void setBurstLengthMs (float ms)
      {
          burstLengthMs_ = juce::jlimit (0.5f, 200.0f, ms);
      }

      /// Level of the burst in dB (negative = attenuated).
      void setBurstLevelDb (float db)
      {
          burstLevel_ = juce::Decibels::decibelsToGain (juce::jlimit (-60.0f, 0.0f, db));
      }

      /// High-pass cutoff frequency for the noise burst.
      void setHPFCutoffHz (float hz)
      {
          hpfCutoffHz_ = juce::jlimit (20.0f, 20000.0f, hz);
          updateHPF();
      }

      /// Call once to schedule a burst that will be injected on the next processBlock() call.
      void triggerBurst() noexcept { burstPhase_ = 0; burstActive_ = true; }

      void reset()
      {
          burstPhase_  = 0;
          burstActive_ = false;
          hpfZ1L_ = hpfZ1R_ = 0.0f;
      }

      // Process stereo block in-place. Burst is mixed additively into the signal.
      void processBlock (float* left, float* right, int numSamples)
      {
          if (!burstActive_) return;

          int burstLenSamps = (int) std::ceil (burstLengthMs_ * 0.001 * sr_);

          for (int i = 0; i < numSamples && burstActive_; ++i)
          {
              if (burstPhase_ >= burstLenSamps)
              {
                  burstActive_ = false;
                  break;
              }

              // White noise via LCG
              rngState_ = rngState_ * 6364136223846793005ULL + 1442695040888963407ULL;
              float noise = (float) (int32_t) (rngState_ >> 33) * (1.0f / 1073741824.0f);

              // 1-pole HP filter: y[n] = x[n] - x[n-1] + coeff * y[n-1]
              float hpL = noise - hpfZ1L_ + hpfCoeff_ * hpfZ1L_;
              hpfZ1L_  = noise;
              float hpR = noise - hpfZ1R_ + hpfCoeff_ * hpfZ1R_;
              hpfZ1R_  = noise;

              // Amplitude envelope: linear decay over burst length
              float env = 1.0f - (float) burstPhase_ / (float) burstLenSamps;

              left[i]  += hpL * env * burstLevel_;
              right[i] += hpR * env * burstLevel_;

              ++burstPhase_;
          }
      }

  private:
      void updateHPF()
      {
          // 1-pole matched-Z HP coefficient
          hpfCoeff_ = std::exp (-2.0f * juce::MathConstants<float>::pi * hpfCutoffHz_ / (float) sr_);
      }

      double   sr_           = 44100.0;
      float    burstLengthMs_ = 5.0f;
      float    burstLevel_    = juce::Decibels::decibelsToGain (-24.0f);
      float    hpfCutoffHz_   = 4000.0f;
      float    hpfCoeff_      = 0.0f;
      float    hpfZ1L_        = 0.0f;
      float    hpfZ1R_        = 0.0f;
      uint64_t rngState_      = 0xDEADBEEFCAFEBABEULL;

      int  burstPhase_  = 0;
      bool burstActive_ = false;
  };
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add NoiseBurst to RebirthDSP.h — HP-filtered transient injector for ONSET profile`

---

- [ ] **Step 1.4 — FormantResonator**

  Append `FormantResonator` to `RebirthDSP.h` inside the namespace.

  ```cpp
  //==============================================================================
  // FormantResonator — FFT formant estimator + parallel bandpass resonator bank.
  //
  // Step 1: Analyze the entire source buffer with a windowed 2048-point FFT.
  //         Pick the top 4 spectral peaks as formant center frequencies.
  // Step 2: Build 4 parallel second-order bandpass filters (biquads) tuned to
  //         those frequencies with configurable Q.
  // Step 3: processBlock() applies the resonator bank: dry signal passes through
  //         each resonator in parallel, outputs are summed and blended with dry.
  //
  // Implementation note: NOT LPC. LPC is too expensive for <3s preview target.
  // Windowed FFT peak picking is sufficient for perceptual formant capture.
  //
  // OPERA profile uses this to give samples vocal/choral shimmer quality.
  //==============================================================================
  class FormantResonator
  {
  public:
      static constexpr int kNumFormants = 4;
      static constexpr int kFFTSize     = 2048;

      FormantResonator() = default;

      void prepare (double sampleRate, int /*maxBlockSize*/)
      {
          sr_  = sampleRate;
          q_   = 8.0f;
          mix_ = 0.4f;
          // Default formant frequencies (vowel "ah" approximation)
          formantHz_[0] = 800.0f;
          formantHz_[1] = 1200.0f;
          formantHz_[2] = 2500.0f;
          formantHz_[3] = 3500.0f;
          rebuildFilters();
          reset();
      }

      /// Q of all resonators (4.0 = wide/warm, 16.0 = narrow/bright).
      void setQ (float q)
      {
          q_ = juce::jlimit (0.5f, 50.0f, q);
          rebuildFilters();
      }

      /// Mix of resonator bank (0 = dry, 1 = full resonator output).
      void setMix (float m) { mix_ = juce::jlimit (0.0f, 1.0f, m); }

      /// Analyze a buffer to extract formant frequencies.
      /// Call this once before processBlock(). Uses the mono sum of the buffer.
      void analyzeFormants (const juce::AudioBuffer<float>& buf, double /*sr*/)
      {
          int totalSamps = buf.getNumSamples();
          int numCh      = buf.getNumChannels();
          if (totalSamps < kFFTSize) return;

          // Build mono sum into FFT input (use first kFFTSize samples from center-ish)
          int startSamp = std::max (0, (totalSamps - kFFTSize) / 2);
          std::vector<float> window (kFFTSize, 0.0f);
          for (int i = 0; i < kFFTSize; ++i)
          {
              float s = 0.0f;
              for (int ch = 0; ch < numCh; ++ch)
                  s += buf.getSample (ch, startSamp + i);
              s /= (float) numCh;
              // Hann window
              float w = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (kFFTSize - 1)));
              window[i] = s * w;
          }

          // Real FFT using juce::dsp::FFT
          std::vector<std::complex<float>> fftData (kFFTSize);
          for (int i = 0; i < kFFTSize; ++i)
              fftData[i] = { window[i], 0.0f };

          // Manual DFT for the top N/2 bins — only need magnitudes for peak picking.
          // Using juce::dsp::FFT requires the buffer to be 2*N floats (interleaved).
          std::vector<float> fftBuf (kFFTSize * 2, 0.0f);
          for (int i = 0; i < kFFTSize; ++i) fftBuf[i * 2] = window[i];

          juce::dsp::FFT fft (11); // 2^11 = 2048
          fft.performRealOnlyForwardTransform (fftBuf.data());

          // Compute magnitude spectrum (first N/2 bins)
          int halfN = kFFTSize / 2;
          std::vector<float> mag (halfN, 0.0f);
          for (int i = 0; i < halfN; ++i)
          {
              float re = fftBuf[i * 2];
              float im = fftBuf[i * 2 + 1];
              mag[i]   = std::sqrt (re * re + im * im);
          }

          // Simple peak picking: find top kNumFormants local maxima.
          // Min distance between peaks: 100 Hz in bin space.
          float binHz = (float) sr_ / (float) kFFTSize;
          int   minBinGap = std::max (1, (int) (100.0f / binHz));

          // Ignore DC and sub-100Hz bins
          int minBin = std::max (1, (int) (100.0f / binHz));
          int maxBin = halfN - 1;

          struct Peak { int bin; float mag; };
          std::vector<Peak> peaks;

          for (int i = minBin + 1; i < maxBin - 1; ++i)
          {
              if (mag[i] > mag[i - 1] && mag[i] > mag[i + 1])
                  peaks.push_back ({ i, mag[i] });
          }

          // Sort by magnitude descending
          std::sort (peaks.begin(), peaks.end(),
                     [] (const Peak& a, const Peak& b) { return a.mag > b.mag; });

          // Select top kNumFormants peaks with minimum bin gap enforcement
          std::vector<int> selectedBins;
          for (const auto& p : peaks)
          {
              bool tooClose = false;
              for (int sel : selectedBins)
                  if (std::abs (p.bin - sel) < minBinGap) { tooClose = true; break; }
              if (!tooClose)
              {
                  selectedBins.push_back (p.bin);
                  if ((int) selectedBins.size() >= kNumFormants) break;
              }
          }

          // Pad with default frequencies if fewer than kNumFormants peaks found
          static const float kDefaults[kNumFormants] = { 800.0f, 1200.0f, 2500.0f, 3500.0f };
          for (int i = 0; i < kNumFormants; ++i)
          {
              if (i < (int) selectedBins.size())
                  formantHz_[i] = (float) selectedBins[i] * binHz;
              else
                  formantHz_[i] = kDefaults[i];
          }

          // Sort formants ascending (perceptual convention)
          std::sort (formantHz_, formantHz_ + kNumFormants);
          rebuildFilters();
      }

      void reset()
      {
          for (int f = 0; f < kNumFormants; ++f)
              for (int ch = 0; ch < 2; ++ch)
                  z1_[f][ch] = z2_[f][ch] = 0.0f;
      }

      // Process stereo block in-place. Each sample is fed through the parallel
      // resonator bank; outputs are summed and blended with the dry signal.
      void processBlock (float* left, float* right, int numSamples)
      {
          if (mix_ < 0.001f) return;

          for (int i = 0; i < numSamples; ++i)
          {
              float dryL = left[i];
              float dryR = right[i];
              float wetL = 0.0f;
              float wetR = 0.0f;

              // Parallel bandpass resonators
              for (int f = 0; f < kNumFormants; ++f)
              {
                  // Transposed Direct Form II biquad
                  float yL = b0_[f] * dryL + z1_[f][0];
                  z1_[f][0] = b1_[f] * dryL - a1_[f] * yL + z2_[f][0];
                  z2_[f][0] = b2_[f] * dryL - a2_[f] * yL;
                  wetL += yL;

                  float yR = b0_[f] * dryR + z1_[f][1];
                  z1_[f][1] = b1_[f] * dryR - a1_[f] * yR + z2_[f][1];
                  z2_[f][1] = b2_[f] * dryR - a2_[f] * yR;
                  wetR += yR;
              }

              // Normalize parallel sum (kNumFormants resonators)
              wetL /= (float) kNumFormants;
              wetR /= (float) kNumFormants;

              left[i]  = dryL + mix_ * (wetL - dryL);
              right[i] = dryR + mix_ * (wetR - dryR);
          }
      }

  private:
      void rebuildFilters()
      {
          // Constant-peak-gain bandpass (BPF) biquad coefficients.
          // At resonance: gain = 1.0 (0 dB). Two zeros at DC and Nyquist.
          for (int f = 0; f < kNumFormants; ++f)
          {
              float fc = formantHz_[f];
              float w0 = 2.0f * juce::MathConstants<float>::pi * fc / (float) sr_;
              float alpha = std::sin (w0) / (2.0f * q_);

              float cosW0 = std::cos (w0);
              float a0inv = 1.0f / (1.0f + alpha);

              b0_[f] =  alpha * a0inv;
              b1_[f] =  0.0f;
              b2_[f] = -alpha * a0inv;
              a1_[f] = -2.0f * cosW0 * a0inv;
              a2_[f] =  (1.0f - alpha) * a0inv;
          }
      }

      double sr_  = 44100.0;
      float  q_   = 8.0f;
      float  mix_ = 0.4f;

      float formantHz_[kNumFormants] = { 800.0f, 1200.0f, 2500.0f, 3500.0f };

      // Biquad coefficients per formant
      float b0_[kNumFormants] = {};
      float b1_[kNumFormants] = {};
      float b2_[kNumFormants] = {};
      float a1_[kNumFormants] = {};
      float a2_[kNumFormants] = {};

      // State: z1[formant][channel], z2[formant][channel]
      float z1_[kNumFormants][2] = {};
      float z2_[kNumFormants][2] = {};
  };

  } // namespace xolokun
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add FormantResonator to RebirthDSP.h — FFT peak detection + biquad resonator bank`

---

### Task 2: K-Weighted LUFS Helper (parallel with Tasks 1–3)

**What:** Create `Source/Export/RebirthLUFS.h` containing a standalone K-weighted LUFS measurement function. This replaces the approximate RMS-based LUFS used in Phase 1A with proper ITU-R BS.1770-4 K-weighting.

**Why:** Used by RebirthPipeline for input normalization (Step 2) and output compensation (Step 5). Can be developed in parallel with Tasks 1–3 since it has no dependency on other new Rebirth files. Also upgrades the existing `XOutshine::normalize()` path for consistency.

**Files:**
- Create: `Source/Export/RebirthLUFS.h`

---

- [ ] **Step 2.1 — Implement K-weighted LUFS measurement**

  Create `Source/Export/RebirthLUFS.h`:

  ```cpp
  #pragma once
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <cmath>
  #include <vector>

  namespace xolokun {

  //==============================================================================
  // RebirthLUFS — ITU-R BS.1770-4 K-weighted integrated loudness measurement.
  //
  // Algorithm:
  //   1. Pre-filter stage 1: 2nd-order high-shelf (+4 dB above ~1.5kHz)
  //   2. Pre-filter stage 2: 2nd-order high-pass (fc ≈ 38Hz, RLB weighting)
  //   3. Mean-square measurement on K-weighted signal
  //   4. Integration: -0.691 + 10*log10(sum_of_mean_square) LUFS
  //
  // Coefficients are for 48kHz reference and are rescaled using bilinear
  // transform prewarp for non-48kHz sample rates — never hardcoded.
  //
  // Returns integrated loudness in LUFS (negative value; -14 = loud, -23 = quiet).
  //==============================================================================

  struct KWeightingFilter
  {
      // Transposed Direct Form II biquad state (stereo)
      float z1[2] = {};
      float z2[2] = {};

      float b0 = 1.0f, b1 = 0.0f, b2 = 0.0f;
      float a1 = 0.0f, a2 = 0.0f;

      void setCoeffs (float b0_, float b1_, float b2_, float a1_, float a2_)
      {
          b0 = b0_; b1 = b1_; b2 = b2_; a1 = a1_; a2 = a2_;
          z1[0] = z1[1] = z2[0] = z2[1] = 0.0f;
      }

      float processSample (float x, int ch)
      {
          float y = b0 * x + z1[ch];
          z1[ch]  = b1 * x - a1 * y + z2[ch];
          z2[ch]  = b2 * x - a2 * y;
          return y;
      }
  };

  //------------------------------------------------------------------------------
  // Build K-weighting filter pair for a given sample rate.
  // Stage 1 = high-shelf pre-filter; Stage 2 = high-pass RLB weighting.
  // Coefficients derived from BS.1770 reference (48kHz) via bilinear prewarp.
  //------------------------------------------------------------------------------
  inline void buildKWeightingFilters (double sampleRate,
                                      KWeightingFilter& stage1,
                                      KWeightingFilter& stage2)
  {
      // --- Stage 1: High-shelf pre-filter ---
      // BS.1770 reference @ 48kHz:
      //   b0 =  1.53512485958697, b1 = -2.69169618940638, b2 =  1.19839281085285
      //   a1 = -1.69065929318241, a2 =  0.73248077421585
      // Prewarp to target sample rate using the pole/zero mapping via fc.
      // The filter's characteristic frequency is approximately 1681.97 Hz.
      // We use bilinear transform: compute analog prototype poles/zeros, then map.
      // For correctness across sample rates, apply the standard bilinear frequency
      // prewarp: Wc_d = 2*sr*tan(pi*fc/sr), where fc ≈ 1681.97 Hz for stage 1.
      {
          constexpr double fc1   = 1681.97;
          double wc  = 2.0 * sampleRate * std::tan (juce::MathConstants<double>::pi * fc1 / sampleRate);
          // Analog high-shelf (gain ≈ +3.999 dB at high frequencies):
          // H(s) = (s^2 + Vg * wc/Q * s + wc^2) / (s^2 + wc/Q * s + wc^2)
          // Q = 0.5, Vg = sqrt(10^(3.999/20)) ≈ 1.5848
          constexpr double Q    = 0.5;
          constexpr double Vg   = 1.58489319; // 10^(4/20)
          double wc2 = wc * wc;
          double sr2 = 4.0 * sampleRate * sampleRate;
          double a0  = sr2 + wc / Q * sampleRate * 2.0 + wc2;

          // Bilinear transform of analog high-shelf
          double b0  = (sr2 + Vg * wc / Q * sampleRate * 2.0 + wc2) / a0;
          double b1  = 2.0 * (wc2 - sr2) / a0;
          double b2  = (sr2 - Vg * wc / Q * sampleRate * 2.0 + wc2) / a0;
          double a1  = 2.0 * (wc2 - sr2) / a0;
          double a2  = (sr2 - wc / Q * sampleRate * 2.0 + wc2) / a0;

          stage1.setCoeffs ((float) b0, (float) b1, (float) b2, (float) a1, (float) a2);
      }

      // --- Stage 2: RLB high-pass weighting ---
      // BS.1770 reference: 2nd-order Butterworth HP, fc = 38.135 Hz
      {
          constexpr double fc2  = 38.135;
          double wc  = 2.0 * sampleRate * std::tan (juce::MathConstants<double>::pi * fc2 / sampleRate);
          double wc2 = wc * wc;
          double sr2 = 4.0 * sampleRate * sampleRate;
          constexpr double Q = juce::MathConstants<double>::sqrt2 * 0.5; // Butterworth 2nd-order
          double a0  = sr2 + wc / Q * sampleRate * 2.0 + wc2;

          // Butterworth HP: b0 = sr2, b1 = -2*sr2, b2 = sr2
          double b0  = sr2 / a0;
          double b1  = -2.0 * sr2 / a0;
          double b2  = sr2 / a0;
          double a1  = 2.0 * (wc2 - sr2) / a0;
          double a2  = (sr2 - wc / Q * sampleRate * 2.0 + wc2) / a0;

          stage2.setCoeffs ((float) b0, (float) b1, (float) b2, (float) a1, (float) a2);
      }
  }

  //------------------------------------------------------------------------------
  // computeIntegratedLUFS() — measure integrated loudness of a buffer.
  // Returns LUFS value (typically -40.0 to 0.0). Returns -100.0 for silence.
  //------------------------------------------------------------------------------
  inline float computeIntegratedLUFS (const juce::AudioBuffer<float>& buffer,
                                      double sampleRate)
  {
      int numCh    = buffer.getNumChannels();
      int numSamps = buffer.getNumSamples();
      if (numSamps == 0 || numCh == 0) return -100.0f;

      KWeightingFilter s1, s2;
      buildKWeightingFilters (sampleRate, s1, s2);

      double sumMeanSquare = 0.0;
      int    count         = 0;

      // Process in 100ms blocks for gating (absolute gate = -70 LUFS threshold)
      constexpr double kGateThresholdLUFS = -70.0;
      int blockSamps = std::max (1, (int) (sampleRate * 0.1));

      for (int blockStart = 0; blockStart < numSamps; blockStart += blockSamps)
      {
          int blockEnd = std::min (blockStart + blockSamps, numSamps);
          double blockMeanSquare = 0.0;

          for (int i = blockStart; i < blockEnd; ++i)
          {
              float kw = 0.0f;
              // Average over channels (BS.1770 per-channel mean square then average)
              for (int ch = 0; ch < numCh; ++ch)
              {
                  float x = buffer.getSample (ch, i);
                  float y = s1.processSample (x, ch % 2);
                  float z = s2.processSample (y, ch % 2);
                  kw += z * z;
              }
              kw /= (float) numCh;
              blockMeanSquare += kw;
          }
          blockMeanSquare /= (double) (blockEnd - blockStart);

          // Absolute gate: ignore blocks below -70 LUFS
          double blockLUFS = -0.691 + 10.0 * std::log10 (std::max (1e-30, blockMeanSquare));
          if (blockLUFS > kGateThresholdLUFS)
          {
              sumMeanSquare += blockMeanSquare;
              ++count;
          }
      }

      if (count == 0) return -100.0f;

      double integrated = -0.691 + 10.0 * std::log10 (sumMeanSquare / (double) count);
      return (float) integrated;
  }

  } // namespace xolokun
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add RebirthLUFS.h — ITU-R BS.1770-4 K-weighted integrated loudness measurement`

---

### Task 3: RebirthProfiles.h — Profile Data Structures and 5 Profile Definitions

**What:** Create `Source/Export/RebirthProfiles.h` defining `RebirthProfileID`, `RebirthDSPModuleID`, `DSPModuleConfig`, `RebirthProfile`, and the 5 built-in profile definitions for OBRIX, ONSET, OWARE, OPERA, and OVERWASH.

**Why:** The pipeline (Task 4) iterates profiles generically — no `if (profile == X)` conditionals. Profiles are pure data. Depends on `RebirthDSP.h` for the `RebirthDSPModuleID` enum values.

**Files:**
- Create: `Source/Export/RebirthProfiles.h`

---

- [ ] **Step 3.1 — Enums, config struct, and profile struct**

  Create `Source/Export/RebirthProfiles.h`:

  ```cpp
  #pragma once
  #include "RebirthDSP.h"

  #include <string>
  #include <unordered_map>
  #include <utility>
  #include <vector>

  namespace xolokun {

  //==============================================================================
  // RebirthProfileID — The 5 engine-inspired transformation profiles.
  //==============================================================================
  enum class RebirthProfileID
  {
      OBRIX,      ///< Harmonic Character — reef-modal saturation + comb + reverb
      ONSET,      ///< Percussive Crunch — transient shaping + noise burst + filter
      OWARE,      ///< Resonant Body — Akan-interval comb bank + sympathetic resonance
      OPERA,      ///< Harmonic Shimmer — FFT formant estimation + resonator bank
      OVERWASH    ///< Deep Diffusion — allpass cascade + spectral blur + LFO
  };

  //==============================================================================
  // RebirthDSPModuleID — All DSP modules the pipeline can instantiate.
  // Modules prefixed with a comment indicate where they are implemented.
  //==============================================================================
  enum class RebirthDSPModuleID
  {
      Saturator,          ///< Source/DSP/Effects/Saturator.h
      Combulator,         ///< Source/DSP/Effects/Combulator.h
      TransientDesigner,  ///< Source/DSP/Effects/TransientDesigner.h
      BrickwallLimiter,   ///< Source/DSP/Effects/BrickwallLimiter.h (used in pipeline, not in chains)
      LushReverb,         ///< Source/DSP/Effects/LushReverb.h
      SpectralTilt,       ///< Source/DSP/Effects/SpectralTilt.h
      AllpassDiffuser,    ///< Source/Export/RebirthDSP.h
      FormantResonator,   ///< Source/Export/RebirthDSP.h
      NoiseBurst,         ///< Source/Export/RebirthDSP.h
      SoftClipGuard,      ///< Source/Export/RebirthDSP.h — inline, no module instance needed
      BiquadLPFilter,     ///< Inline biquad LP (Butterworth 2nd-order) — used by ONSET and OVERWASH
      WetDryMix           ///< Final wet/dry blend using the intensity parameter
  };

  //==============================================================================
  // DSPModuleConfig — One module entry in a profile's FX chain.
  // 'params' hold base parameter values. 'velocityScale' holds {v0, v1} ranges
  // for linear interpolation: value = v0 + velocityNorm * (v1 - v0).
  //==============================================================================
  struct DSPModuleConfig
  {
      RebirthDSPModuleID moduleId;
      std::unordered_map<std::string, float>                       params;
      std::unordered_map<std::string, std::pair<float, float>>     velocityScale;
  };

  //==============================================================================
  // RebirthSettings — embedded in OutshineSettings (added in Task 5).
  //==============================================================================
  struct RebirthSettings
  {
      RebirthProfileID profileId    = RebirthProfileID::OBRIX;
      float            intensity    = 0.7f;   ///< 0.0 = dry, 1.0 = full transformation
      float            chaosAmount  = 0.0f;   ///< Phase 1B: always ignored (Phase 1C)
      bool             enabled      = false;
  };

  //==============================================================================
  // RebirthProfile — complete description of one engine-inspired transformation.
  //==============================================================================
  struct RebirthProfile
  {
      RebirthProfileID   id;
      const char*        engineName;              ///< "OBRIX", "ONSET", etc.
      const char*        producerLabel;           ///< Human-facing name shown in UI
      const char*        materialCategory;        ///< "TONAL", "PERCUSSIVE", etc.
      const char*        description;             ///< 1-line sonic description for UI
      const char*        characterBrief;          ///< Acceptance test: what does it sound like?
      const char*        intensityDescription;    ///< Per-profile intensity label for UI
      float              recommendedTransientMax; ///< Warn if sample transient ratio exceeds this
      float              tailSeconds;             ///< Extra render time after sample ends
      std::vector<DSPModuleConfig> chain;         ///< Ordered DSP module list
  };

  //==============================================================================
  // getRebirthProfile() — returns the built-in profile for the given ID.
  //==============================================================================
  inline const RebirthProfile& getRebirthProfile (RebirthProfileID id)
  {
      // --- OBRIX: Harmonic Character ---
      static const RebirthProfile OBRIX_PROFILE = {
          RebirthProfileID::OBRIX,
          "OBRIX",
          "Harmonic Character",
          "TONAL",
          "Reef-modal saturation + comb resonance + ecological reverb",
          "Sample is recognizable but richer — dense overtone structure, comb shimmer",
          "Controls harmonic density and comb resonance",
          0.7f,  // warn if transient ratio > 0.7 (percussive material may over-saturate)
          0.5f,  // 0.5s reverb tail
          {
              // Step 1: Saturator — Tube mode, vel-scaled drive
              {
                  RebirthDSPModuleID::Saturator,
                  { {"drive", 0.4f}, {"mode", 0.0f} /*Tube=0*/ },
                  { {"drive", {0.2f, 0.8f}} }
              },
              // Step 2: SoftClipGuard — no params, applied inline after saturator
              {
                  RebirthDSPModuleID::SoftClipGuard,
                  {}, {}
              },
              // Step 3: Combulator — base feedback 0.85, vel-scaled 0.75→0.95
              {
                  RebirthDSPModuleID::Combulator,
                  { {"feedback", 0.85f}, {"damping", 0.3f}, {"mix", 0.4f} },
                  { {"feedback", {0.75f, 0.95f}}, {"damping", {0.5f, 0.1f}} }
              },
              // Step 4: LushReverb — decay 0.4s, vel-scaled mix 0.15→0.35
              {
                  RebirthDSPModuleID::LushReverb,
                  { {"roomSize", 0.5f}, {"damping", 0.4f}, {"mix", 0.25f} },
                  { {"mix", {0.15f, 0.35f}} }
              },
              // Step 5: WetDryMix — uses intensity parameter
              {
                  RebirthDSPModuleID::WetDryMix,
                  {}, {}
              }
          }
      };

      // --- ONSET: Percussive Crunch ---
      static const RebirthProfile ONSET_PROFILE = {
          RebirthProfileID::ONSET,
          "ONSET",
          "Percussive Crunch",
          "PERCUSSIVE",
          "Transient emphasis + noise burst + filter sweep",
          "Attack is sculpted and crackles with noise — body is filter-swept and punchy",
          "Controls transient aggression and noise level",
          1.0f,  // no warning — designed for percussive material
          0.15f,
          {
              // Step 1: TransientDesigner — vel-scaled attack boost +2→+12 dB
              {
                  RebirthDSPModuleID::TransientDesigner,
                  { {"attack", 0.5f /*= +6dB base*/}, {"sustain", -0.25f} },
                  { {"attack", {0.167f, 1.0f}} }  // maps to +2dB→+12dB
              },
              // Step 2: NoiseBurst — vel-scaled level -30→-18 dBFS
              {
                  RebirthDSPModuleID::NoiseBurst,
                  { {"burstLengthMs", 5.0f}, {"burstLevelDb", -24.0f}, {"hpfCutoffHz", 4000.0f} },
                  { {"burstLevelDb", {-30.0f, -18.0f}} }
              },
              // Step 3: Saturator — Digital mode, vel-scaled drive 0.1→0.6
              {
                  RebirthDSPModuleID::Saturator,
                  { {"drive", 0.3f}, {"mode", 2.0f} /*Digital=2*/ },
                  { {"drive", {0.1f, 0.6f}} }
              },
              // Step 4: BiquadLPFilter — vel-scaled cutoff 8kHz→18kHz
              {
                  RebirthDSPModuleID::BiquadLPFilter,
                  { {"cutoffHz", 13000.0f}, {"q", 0.7f} },
                  { {"cutoffHz", {8000.0f, 18000.0f}} }
              },
              // Step 5: WetDryMix
              {
                  RebirthDSPModuleID::WetDryMix,
                  {}, {}
              }
          }
      };

      // --- OWARE: Resonant Body ---
      // Comb filters tuned to Akan interval sets derived from OwareEngine.h.
      // Interval set selected by spectral centroid (pipeline responsibility).
      static const RebirthProfile OWARE_PROFILE = {
          RebirthProfileID::OWARE,
          "OWARE",
          "Resonant Body",
          "HARMONIC",
          "Akan-interval comb bank + sympathetic resonance + spectral tilt",
          "Sample excites tuned resonant body — pitched ringing, dark shimmer",
          "Controls resonance depth and tonal darkness",
          1.0f,  // no warning
          0.8f,
          {
              // Step 1: Combulator — Akan-tuned (freq set by pipeline from centroid)
              //         vel-scaled feedback 0.8→0.98
              {
                  RebirthDSPModuleID::Combulator,
                  { {"feedback", 0.9f}, {"damping", 0.25f}, {"mix", 0.5f} },
                  { {"feedback", {0.8f, 0.98f}} }
              },
              // Step 2: SoftClipGuard — protect against high-feedback comb resonance
              {
                  RebirthDSPModuleID::SoftClipGuard,
                  {}, {}
              },
              // Step 3: Combulator (sympathetic — 2nd bank at harmonic intervals)
              //         Implemented as a second Combulator chain entry
              {
                  RebirthDSPModuleID::Combulator,
                  { {"feedback", 0.4f}, {"damping", 0.6f}, {"mix", 0.15f} },
                  { {"feedback", {0.3f, 0.5f}} }
              },
              // Step 4: SpectralTilt — vel-scaled: soft = +3dB/oct bright, hard = -2dB/oct dark
              {
                  RebirthDSPModuleID::SpectralTilt,
                  { {"tilt", 0.0f} },
                  { {"tilt", {0.6f, -0.4f}} }  // positive = bright, negative = dark
              },
              // Step 5: WetDryMix
              {
                  RebirthDSPModuleID::WetDryMix,
                  {}, {}
              }
          }
      };

      // --- OPERA: Harmonic Shimmer ---
      static const RebirthProfile OPERA_PROFILE = {
          RebirthProfileID::OPERA,
          "OPERA",
          "Harmonic Shimmer",
          "TONAL",
          "FFT formant estimation + Kuramoto-inspired resonator bank + slow LFO shimmer",
          "Sample gains vocal/choral shimmer — tonal material sounds alive and evolving",
          "Controls resonator Q and spectral shimmer brightness",
          0.5f,  // warn if transient ratio > 0.5 (transient material will have attack blur)
          1.0f,
          {
              // Step 1: FormantResonator — vel-scaled Q 4.0→16.0
              {
                  RebirthDSPModuleID::FormantResonator,
                  { {"q", 8.0f}, {"mix", 0.4f} },
                  { {"q", {4.0f, 16.0f}}, {"mix", {0.3f, 0.5f}} }
              },
              // Step 2: SpectralTilt — vel-scaled: soft = flat (0), hard = +0.4 bright
              {
                  RebirthDSPModuleID::SpectralTilt,
                  { {"tilt", 0.0f} },
                  { {"tilt", {0.0f, 0.4f}} }
              },
              // Step 3: WetDryMix
              {
                  RebirthDSPModuleID::WetDryMix,
                  {}, {}
              }
          }
      };

      // --- OVERWASH: Deep Diffusion ---
      static const RebirthProfile OVERWASH_PROFILE = {
          RebirthProfileID::OVERWASH,
          "OVERWASH",
          "Deep Diffusion",
          "TEXTURAL",
          "Multi-stage allpass diffusion + LP filter + spectral tilt",
          "Transients dissolve into ambient wash — like hearing the sample through deep water",
          "Controls diffusion depth and submersion darkness",
          0.4f,  // warn if transient ratio > 0.4 (percussive attacks will dissolve)
          2.0f,
          {
              // Step 1: AllpassDiffuser — 4 stages, vel-scaled feedback 0.4→0.8
              {
                  RebirthDSPModuleID::AllpassDiffuser,
                  { {"feedback", 0.6f} },
                  { {"feedback", {0.4f, 0.8f}} }
              },
              // Step 2: BiquadLPFilter — vel-scaled cutoff 12kHz→6kHz (harder = darker)
              {
                  RebirthDSPModuleID::BiquadLPFilter,
                  { {"cutoffHz", 9000.0f}, {"q", 0.7f} },
                  { {"cutoffHz", {12000.0f, 6000.0f}} }
              },
              // Step 3: SpectralTilt — vel-scaled: soft = +0.2 bright, hard = -0.6 dark
              {
                  RebirthDSPModuleID::SpectralTilt,
                  { {"tilt", 0.0f} },
                  { {"tilt", {0.2f, -0.6f}} }
              },
              // Step 4: WetDryMix
              {
                  RebirthDSPModuleID::WetDryMix,
                  {}, {}
              }
          }
      };

      switch (id)
      {
          case RebirthProfileID::OBRIX:    return OBRIX_PROFILE;
          case RebirthProfileID::ONSET:    return ONSET_PROFILE;
          case RebirthProfileID::OWARE:    return OWARE_PROFILE;
          case RebirthProfileID::OPERA:    return OPERA_PROFILE;
          case RebirthProfileID::OVERWASH: return OVERWASH_PROFILE;
          default:                         return OBRIX_PROFILE;
      }
  }

  //------------------------------------------------------------------------------
  // Helper: resolve velocity-scaled parameter value.
  // velocityNorm: 0.0 = softest layer, 1.0 = hardest layer.
  //------------------------------------------------------------------------------
  inline float resolveVelocityParam (const DSPModuleConfig& cfg,
                                     const std::string& key,
                                     float velocityNorm)
  {
      auto baseIt = cfg.params.find (key);
      float base  = (baseIt != cfg.params.end()) ? baseIt->second : 0.0f;

      auto scaleIt = cfg.velocityScale.find (key);
      if (scaleIt == cfg.velocityScale.end())
          return base;

      float v0 = scaleIt->second.first;
      float v1 = scaleIt->second.second;
      return v0 + velocityNorm * (v1 - v0);
  }

  //------------------------------------------------------------------------------
  // Helper: auto-select profile for a sample category (spec Section 9.3 table).
  //------------------------------------------------------------------------------
  inline RebirthProfileID autoProfileForCategory (SampleCategory cat)
  {
      switch (cat)
      {
          case SampleCategory::Kick:
          case SampleCategory::Snare:
          case SampleCategory::HiHatClosed:
          case SampleCategory::HiHatOpen:
          case SampleCategory::Clap:
          case SampleCategory::Tom:
          case SampleCategory::Percussion:
              return RebirthProfileID::ONSET;

          case SampleCategory::Bass:
          case SampleCategory::Lead:
          case SampleCategory::Keys:
          case SampleCategory::Pluck:
          case SampleCategory::String:
              return RebirthProfileID::OBRIX;

          case SampleCategory::Vocal:
          case SampleCategory::Woodwind:
          case SampleCategory::Brass:
              return RebirthProfileID::OPERA;

          case SampleCategory::Pad:
              return RebirthProfileID::OVERWASH;

          default:
              return RebirthProfileID::OBRIX;  // FX, Loop, Unknown — general-purpose default
      }
  }

  } // namespace xolokun
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add RebirthProfiles.h — 5 engine-inspired profile definitions (OBRIX/ONSET/OWARE/OPERA/OVERWASH)`

---

### Task 4: RebirthPipeline.h — Pipeline Orchestrator

**What:** Create `Source/Export/RebirthPipeline.h` with the `AnalysisResult` struct and `RebirthPipeline` class that orchestrates all pipeline stages: resample, LUFS normalize, analyze, apply FX chain, compensate gain, apply true-peak limit.

**Why:** Depends on RebirthDSP.h, RebirthProfiles.h, and RebirthLUFS.h. Contains no profile-specific conditional logic — iterates profile chain data generically. Used by XOutshine::enhance() in Task 5.

**Files:**
- Create: `Source/Export/RebirthPipeline.h`

---

- [ ] **Step 4.1 — AnalysisResult struct and class skeleton**

  Create `Source/Export/RebirthPipeline.h` with includes, `AnalysisResult`, and the `RebirthPipeline` class declaration:

  ```cpp
  #pragma once
  #include <juce_audio_basics/juce_audio_basics.h>
  #include <juce_dsp/juce_dsp.h>

  #include "../DSP/Effects/Saturator.h"
  #include "../DSP/Effects/Combulator.h"
  #include "../DSP/Effects/BrickwallLimiter.h"
  #include "../DSP/Effects/TransientDesigner.h"
  #include "../DSP/Effects/SpectralTilt.h"
  #include "../DSP/Effects/LushReverb.h"
  #include "RebirthDSP.h"
  #include "RebirthProfiles.h"
  #include "RebirthLUFS.h"

  #include <atomic>
  #include <cmath>
  #include <functional>
  #include <vector>

  namespace xolokun {

  //==============================================================================
  // AnalysisResult — audio feature vector computed from a sample buffer.
  // Used by the pipeline to auto-set profile parameters.
  //==============================================================================
  struct AnalysisResult
  {
      float transientRatio  = 0.5f;   ///< 0.0 = sustained, 1.0 = sharp attack
      float spectralCentroid = 1000.0f; ///< Hz — brightness indicator
      float spectralFlatness = 0.5f;  ///< 0.0 = pure tone, 1.0 = white noise
      float durationS        = 1.0f;  ///< sample duration in seconds
  };

  //==============================================================================
  // RebirthPipeline — generic DSP chain executor for engine-inspired sample
  // transformation. All profile-specific logic lives in RebirthProfiles.h data
  // structures; this class contains NO per-profile conditional branches.
  //==============================================================================
  class RebirthPipeline
  {
  public:
      RebirthPipeline() = default;

      //--------------------------------------------------------------------------
      // Process a single sample buffer through the Rebirth FX chain.
      // Returns a new buffer with the transformed audio at targetSampleRate.
      // velocityNorm: 0.0 = softest layer, 1.0 = hardest — drives spectral scaling.
      // rng: seeded juce::Random for per-round-robin variation.
      // progress: optional callback invoked with 0.0→1.0 during processing.
      //--------------------------------------------------------------------------
      juce::AudioBuffer<float> process (
          const juce::AudioBuffer<float>& source,
          double sourceSampleRate,
          const RebirthSettings& settings,
          float velocityNorm,
          juce::Random& rng,
          std::function<void (float)> progress = nullptr);

      //--------------------------------------------------------------------------
      // Preview: process first 2 seconds only, full quality.
      // Target: <3 seconds wall time on Apple M1.
      //--------------------------------------------------------------------------
      juce::AudioBuffer<float> preview (
          const juce::AudioBuffer<float>& source,
          double sourceSampleRate,
          const RebirthSettings& settings);

      //--------------------------------------------------------------------------
      // Cancel a running process() or preview() call (thread-safe).
      // The cancelled call returns an empty buffer.
      //--------------------------------------------------------------------------
      void cancel() noexcept { cancelFlag_.store (true); }

  private:
      std::atomic<bool> cancelFlag_ { false };

      // Target sample rate for Rebirth processing (44100.0 Hz engine convention)
      static constexpr double kTargetSampleRate = 44100.0;

      // Pipeline stages (called sequentially by process())
      juce::AudioBuffer<float> resampleBuffer (
          const juce::AudioBuffer<float>& source,
          double sourceSR,
          double targetSR);

      float computeLUFS (
          const juce::AudioBuffer<float>& buffer,
          double sampleRate);

      AnalysisResult analyzeAudio (
          const juce::AudioBuffer<float>& buffer,
          double sampleRate);

      juce::AudioBuffer<float> applyChain (
          const juce::AudioBuffer<float>& buffer,
          double sampleRate,
          const RebirthProfile& profile,
          float velocityNorm,
          const AnalysisResult& analysis,
          juce::Random& rng);

      void compensateGain (
          juce::AudioBuffer<float>& buffer,
          double sampleRate,
          float targetLUFS);

      void applyTruePeakLimit (
          juce::AudioBuffer<float>& buffer,
          double sampleRate);
  };
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add RebirthPipeline.h skeleton — AnalysisResult struct and class declaration`

---

- [ ] **Step 4.2 — Implement resampleBuffer() and analyzeAudio()**

  Add the private method implementations to `RebirthPipeline` in `RebirthPipeline.h` (inline in the header, inside the class after the declaration or as out-of-line inline definitions below the class within the same header).

  ```cpp
  //==============================================================================
  // RebirthPipeline — inline method implementations
  //==============================================================================

  inline juce::AudioBuffer<float> RebirthPipeline::resampleBuffer (
      const juce::AudioBuffer<float>& source,
      double sourceSR,
      double targetSR)
  {
      if (std::abs (sourceSR - targetSR) < 1.0)
      {
          // No resampling needed — return a copy
          juce::AudioBuffer<float> out (source.getNumChannels(), source.getNumSamples());
          for (int ch = 0; ch < source.getNumChannels(); ++ch)
              out.copyFrom (ch, 0, source, ch, 0, source.getNumSamples());
          return out;
      }

      double ratio = sourceSR / targetSR;
      int outSamps = (int) std::ceil ((double) source.getNumSamples() / ratio);
      int nch      = source.getNumChannels();
      juce::AudioBuffer<float> out (nch, outSamps);

      for (int ch = 0; ch < nch; ++ch)
      {
          juce::LagrangeInterpolator interp;
          interp.reset();
          int written = interp.process (ratio,
                                        source.getReadPointer (ch),
                                        out.getWritePointer (ch),
                                        outSamps,
                                        source.getNumSamples(),
                                        0);
          // Zero any unwritten samples
          if (written < outSamps)
              out.clear (ch, written, outSamps - written);
      }
      return out;
  }

  inline AnalysisResult RebirthPipeline::analyzeAudio (
      const juce::AudioBuffer<float>& buffer,
      double sampleRate)
  {
      AnalysisResult result;
      int numSamps = buffer.getNumSamples();
      int numCh    = buffer.getNumChannels();
      if (numSamps == 0) return result;

      result.durationS = (float) numSamps / (float) sampleRate;

      // --- Transient ratio via onset detection ---
      // Compare attack RMS (first 20ms) to sustain RMS (next 100ms).
      int attackSamps  = std::min ((int) (sampleRate * 0.02), numSamps);
      int sustainEnd   = std::min ((int) (sampleRate * 0.12), numSamps);

      double atkRMS = 0.0, susRMS = 0.0;
      for (int ch = 0; ch < numCh; ++ch)
      {
          auto* ptr = buffer.getReadPointer (ch);
          for (int i = 0; i < attackSamps; ++i)       atkRMS += (double) ptr[i] * ptr[i];
          for (int i = attackSamps; i < sustainEnd; ++i) susRMS += (double) ptr[i] * ptr[i];
      }
      atkRMS /= (double) (attackSamps * numCh);
      int susSamps = sustainEnd - attackSamps;
      if (susSamps > 0) susRMS /= (double) (susSamps * numCh);

      // Transient ratio: how much louder is the attack vs sustain?
      // 0.0 = sustained, 1.0 = pure transient
      if (atkRMS + susRMS > 1e-30)
          result.transientRatio = (float) juce::jlimit (0.0, 1.0, (atkRMS - susRMS) / (atkRMS + susRMS + 1e-30));
      else
          result.transientRatio = 0.0f;

      // --- Spectral centroid and flatness via FFT ---
      // Use first 4096 samples (mono sum) for efficiency
      constexpr int kFFTForAnalysis = 4096;
      int analysisSamps = std::min (kFFTForAnalysis, numSamps);

      std::vector<float> fftBuf (kFFTForAnalysis * 2, 0.0f);
      for (int i = 0; i < analysisSamps; ++i)
      {
          float s = 0.0f;
          for (int ch = 0; ch < numCh; ++ch) s += buffer.getSample (ch, i);
          s /= (float) numCh;
          // Hann window
          float w = 0.5f * (1.0f - std::cos (2.0f * juce::MathConstants<float>::pi * (float) i / (float) (analysisSamps - 1)));
          fftBuf[i * 2] = s * w;
      }

      juce::dsp::FFT fft (12); // 2^12 = 4096
      fft.performRealOnlyForwardTransform (fftBuf.data());

      int halfN = kFFTForAnalysis / 2;
      float binHz = (float) sampleRate / (float) kFFTForAnalysis;

      double sumMag = 0.0, sumMagFreq = 0.0, sumLogMag = 0.0;
      for (int i = 1; i < halfN; ++i)
      {
          float re  = fftBuf[i * 2];
          float im  = fftBuf[i * 2 + 1];
          float mag = std::sqrt (re * re + im * im);
          float hz  = (float) i * binHz;
          sumMag     += mag;
          sumMagFreq += mag * hz;
          sumLogMag  += std::log (std::max (1e-10f, mag));
      }

      if (sumMag > 1e-10)
          result.spectralCentroid = (float) (sumMagFreq / sumMag);
      else
          result.spectralCentroid = 1000.0f;

      // Spectral flatness: exp(mean_log_mag) / mean_mag (geometric/arithmetic ratio)
      float meanMag    = (float) (sumMag / (double) (halfN - 1));
      float geomMean   = std::exp ((float) (sumLogMag / (double) (halfN - 1)));
      if (meanMag > 1e-10f)
          result.spectralFlatness = juce::jlimit (0.0f, 1.0f, geomMean / meanMag);
      else
          result.spectralFlatness = 0.0f;

      return result;
  }
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add resampleBuffer() and analyzeAudio() implementations to RebirthPipeline`

---

- [ ] **Step 4.3 — Implement applyChain(), compensateGain(), applyTruePeakLimit()**

  Append to `RebirthPipeline.h` (inline out-of-class implementations):

  ```cpp
  inline juce::AudioBuffer<float> RebirthPipeline::applyChain (
      const juce::AudioBuffer<float>& buffer,
      double sampleRate,
      const RebirthProfile& profile,
      float velocityNorm,
      const AnalysisResult& analysis,
      juce::Random& rng)
  {
      // Copy input to a working buffer — we process in-place
      juce::AudioBuffer<float> work (buffer.getNumChannels(), buffer.getNumSamples());
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
          work.copyFrom (ch, 0, buffer, ch, 0, buffer.getNumSamples());

      int numCh    = work.getNumChannels();
      int numSamps = work.getNumSamples();

      // Ensure stereo (duplicate mono to L+R if needed)
      if (numCh == 1)
      {
          work.setSize (2, numSamps, true, false, true);
          work.copyFrom (1, 0, work, 0, 0, numSamps);
          numCh = 2;
      }

      float* left  = work.getWritePointer (0);
      float* right = work.getWritePointer (1);

      // Instantiate and configure DSP module instances
      Saturator         saturator;
      Combulator        combulator;
      Combulator        combulator2;   // OWARE sympathetic resonance uses a second comb
      TransientDesigner transient;
      SpectralTilt      spectralTilt;
      LushReverb        lushReverb;
      AllpassDiffuser   allpassDiffuser;
      FormantResonator  formantResonator;
      NoiseBurst        noiseBurst;

      // Prepare all modules at the current sample rate
      saturator.prepare (sampleRate);
      combulator.prepare (sampleRate);
      combulator2.prepare (sampleRate);
      transient.prepare (sampleRate);
      spectralTilt.prepare (sampleRate);
      lushReverb.prepare (sampleRate);
      allpassDiffuser.prepare (sampleRate, numSamps);
      formantResonator.prepare (sampleRate, numSamps);
      noiseBurst.prepare (sampleRate, numSamps);

      // Biquad LP state for BiquadLPFilter module
      float bqZ1L = 0.0f, bqZ1R = 0.0f, bqZ2L = 0.0f, bqZ2R = 0.0f;
      float bqB0 = 1.0f, bqB1 = 0.0f, bqB2 = 0.0f, bqA1 = 0.0f, bqA2 = 0.0f;
      auto buildBiquadLP = [&] (float cutoffHz, float q)
      {
          float w0     = 2.0f * juce::MathConstants<float>::pi * cutoffHz / (float) sampleRate;
          float sinW0  = std::sin (w0);
          float cosW0  = std::cos (w0);
          float alpha  = sinW0 / (2.0f * q);
          float a0inv  = 1.0f / (1.0f + alpha);
          bqB0 = (1.0f - cosW0) * 0.5f * a0inv;
          bqB1 = (1.0f - cosW0) * a0inv;
          bqB2 = bqB0;
          bqA1 = -2.0f * cosW0 * a0inv;
          bqA2 = (1.0f - alpha) * a0inv;
          bqZ1L = bqZ1R = bqZ2L = bqZ2R = 0.0f;
      };
      auto applyBiquadLP = [&] (float* L, float* R, int n)
      {
          for (int i = 0; i < n; ++i)
          {
              float yL = bqB0 * L[i] + bqZ1L;
              bqZ1L    = bqB1 * L[i] - bqA1 * yL + bqZ2L;
              bqZ2L    = bqB2 * L[i] - bqA2 * yL;
              L[i]     = yL;

              float yR = bqB0 * R[i] + bqZ1R;
              bqZ1R    = bqB1 * R[i] - bqA1 * yR + bqZ2R;
              bqZ2R    = bqB2 * R[i] - bqA2 * yR;
              R[i]     = yR;
          }
      };

      // FormantResonator: analyze source buffer before chain (OPERA profile)
      bool formantAnalyzed = false;
      for (const auto& mod : profile.chain)
          if (mod.moduleId == RebirthDSPModuleID::FormantResonator && !formantAnalyzed)
          {
              formantResonator.analyzeFormants (buffer, sampleRate);
              formantAnalyzed = true;
          }

      // OWARE: auto-select Akan interval set from spectral centroid
      // Akan sets: Wood [1.0, 2.76, 5.40], Metal [1.0, 2.0, 3.0], Bell [1.0, 2.32, 4.18]
      // Use spectral centroid to pick fundamental frequency for comb tuning
      float combFundamental = juce::jlimit (50.0f, 2000.0f, analysis.spectralCentroid * 0.5f);
      float akan_intervals[3][3] = {
          { 1.0f, 2.76f, 5.40f },  // Wood: centroid < 1kHz
          { 1.0f, 2.0f,  3.0f  },  // Metal: 1-4kHz
          { 1.0f, 2.32f, 4.18f }   // Bell: > 4kHz
      };
      int akanSet = (analysis.spectralCentroid < 1000.0f) ? 0
                  : (analysis.spectralCentroid < 4000.0f) ? 1 : 2;

      // Track whether we've configured the second Combulator (OWARE sympathetic)
      int combulatorUseCount = 0;

      // --- Execute the profile chain ---
      for (const auto& mod : profile.chain)
      {
          if (cancelFlag_.load()) break;

          switch (mod.moduleId)
          {
              case RebirthDSPModuleID::Saturator:
              {
                  float drive = resolveVelocityParam (mod, "drive", velocityNorm);
                  float modeF = mod.params.count ("mode") ? mod.params.at ("mode") : 0.0f;
                  saturator.setDrive (drive);
                  saturator.setMode ((Saturator::SaturationMode) (int) modeF);
                  saturator.setMix (1.0f);
                  saturator.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::SoftClipGuard:
              {
                  softClipBlock (left, numSamps);
                  softClipBlock (right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::Combulator:
              {
                  // First Combulator = primary resonance; second = sympathetic (OWARE)
                  Combulator& comb = (combulatorUseCount == 0) ? combulator : combulator2;
                  ++combulatorUseCount;

                  float feedback = resolveVelocityParam (mod, "feedback", velocityNorm);
                  float damping  = mod.params.count ("damping") ? mod.params.at ("damping") : 0.3f;
                  float mix      = mod.params.count ("mix")     ? mod.params.at ("mix")     : 0.4f;

                  // For OWARE: tune combs to Akan intervals
                  if (profile.id == RebirthProfileID::OWARE && combulatorUseCount == 1)
                  {
                      comb.setFrequency (combFundamental);
                      // Tune the 3 combs to Akan interval ratios via offset params
                      // (Combulator uses setOffset1/2 for detuning the 2nd and 3rd combs)
                      comb.setOffset1 (akan_intervals[akanSet][1] - 1.0f);
                      comb.setOffset2 (akan_intervals[akanSet][2] - 1.0f);
                  }
                  else
                  {
                      comb.setFrequency (juce::jlimit (50.0f, 5000.0f, analysis.spectralCentroid));
                  }

                  comb.setFeedback (feedback);
                  comb.setDamping (damping);
                  comb.setMix (mix);
                  comb.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::TransientDesigner:
              {
                  float attack  = resolveVelocityParam (mod, "attack",  velocityNorm);
                  float sustain = mod.params.count ("sustain") ? mod.params.at ("sustain") : 0.0f;
                  transient.setAttack (attack);
                  transient.setSustain (sustain);
                  transient.setMix (1.0f);
                  transient.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::NoiseBurst:
              {
                  float lenMs  = mod.params.count ("burstLengthMs")  ? mod.params.at ("burstLengthMs")  : 5.0f;
                  float levelDb = resolveVelocityParam (mod, "burstLevelDb", velocityNorm);
                  float hpfHz  = mod.params.count ("hpfCutoffHz")    ? mod.params.at ("hpfCutoffHz")    : 4000.0f;
                  noiseBurst.setBurstLengthMs (lenMs);
                  noiseBurst.setBurstLevelDb (levelDb);
                  noiseBurst.setHPFCutoffHz (hpfHz);
                  noiseBurst.triggerBurst();
                  noiseBurst.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::FormantResonator:
              {
                  float q   = resolveVelocityParam (mod, "q",   velocityNorm);
                  float mix = resolveVelocityParam (mod, "mix", velocityNorm);
                  formantResonator.setQ (q);
                  formantResonator.setMix (mix);
                  formantResonator.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::SpectralTilt:
              {
                  float tilt = resolveVelocityParam (mod, "tilt", velocityNorm);
                  spectralTilt.setTilt (tilt);
                  spectralTilt.setMix (1.0f);
                  spectralTilt.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::LushReverb:
              {
                  float roomSize = mod.params.count ("roomSize") ? mod.params.at ("roomSize") : 0.5f;
                  float damping  = mod.params.count ("damping")  ? mod.params.at ("damping")  : 0.4f;
                  float mix      = resolveVelocityParam (mod, "mix", velocityNorm);
                  lushReverb.setRoomSize (roomSize);
                  lushReverb.setDamping (damping);
                  lushReverb.setMix (mix);

                  // LushReverb uses separate in/out buffers
                  std::vector<float> outL (numSamps, 0.0f), outR (numSamps, 0.0f);
                  lushReverb.processBlock (left, right, outL.data(), outR.data(), numSamps);
                  std::copy (outL.begin(), outL.end(), left);
                  std::copy (outR.begin(), outR.end(), right);
                  break;
              }

              case RebirthDSPModuleID::AllpassDiffuser:
              {
                  float feedback = resolveVelocityParam (mod, "feedback", velocityNorm);
                  allpassDiffuser.setFeedbackAll (feedback);
                  allpassDiffuser.processBlock (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::BiquadLPFilter:
              {
                  float cutoff = resolveVelocityParam (mod, "cutoffHz", velocityNorm);
                  float q      = mod.params.count ("q") ? mod.params.at ("q") : 0.7f;
                  buildBiquadLP (cutoff, q);
                  applyBiquadLP (left, right, numSamps);
                  break;
              }

              case RebirthDSPModuleID::WetDryMix:
                  // Applied after the chain — handled in process() using intensity
                  break;

              default:
                  break;
          }
      }

      return work;
  }

  inline void RebirthPipeline::compensateGain (
      juce::AudioBuffer<float>& buffer,
      double sampleRate,
      float targetLUFS)
  {
      float currentLUFS = computeLUFS (buffer, sampleRate);
      if (currentLUFS <= -99.0f) return;

      float gainDb   = targetLUFS - currentLUFS;
      float gainLin  = juce::Decibels::decibelsToGain (juce::jlimit (-40.0f, 40.0f, gainDb));
      buffer.applyGain (gainLin);
  }

  inline void RebirthPipeline::applyTruePeakLimit (
      juce::AudioBuffer<float>& buffer,
      double sampleRate)
  {
      BrickwallLimiter limiter;
      limiter.prepare (sampleRate, buffer.getNumSamples());
      limiter.setCeiling (-1.0f);  // -1.0 dBTP

      if (buffer.getNumChannels() >= 2)
          limiter.processBlock (buffer.getWritePointer (0), buffer.getWritePointer (1), buffer.getNumSamples());
      else if (buffer.getNumChannels() == 1)
      {
          float* ch0 = buffer.getWritePointer (0);
          // Mono: process L+R as same pointer (limiter handles gracefully)
          limiter.processBlock (ch0, ch0, buffer.getNumSamples());
      }
  }

  inline float RebirthPipeline::computeLUFS (
      const juce::AudioBuffer<float>& buffer,
      double sampleRate)
  {
      return computeIntegratedLUFS (buffer, sampleRate);
  }
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add applyChain(), compensateGain(), applyTruePeakLimit() to RebirthPipeline`

---

- [ ] **Step 4.4 — Implement process() and preview()**

  Append the `process()` and `preview()` top-level method implementations to `RebirthPipeline.h`:

  ```cpp
  inline juce::AudioBuffer<float> RebirthPipeline::process (
      const juce::AudioBuffer<float>& source,
      double sourceSampleRate,
      const RebirthSettings& settings,
      float velocityNorm,
      juce::Random& rng,
      std::function<void (float)> progress)
  {
      cancelFlag_.store (false);

      if (!settings.enabled || source.getNumSamples() == 0)
          return source;  // passthrough when disabled

      const RebirthProfile& profile = getRebirthProfile (settings.profileId);

      if (progress) progress (0.05f);

      // Step 1: Resample to target SR
      double targetSR = kTargetSampleRate;
      juce::AudioBuffer<float> resampled = resampleBuffer (source, sourceSampleRate, targetSR);
      if (cancelFlag_.load()) return {};

      if (progress) progress (0.15f);

      // Step 2: Input normalization to -18 LUFS
      constexpr float kInputLUFSTarget = -18.0f;
      float inputLUFS = computeLUFS (resampled, targetSR);
      compensateGain (resampled, targetSR, kInputLUFSTarget);

      if (cancelFlag_.load()) return {};
      if (progress) progress (0.25f);

      // Step 3: Audio analysis
      AnalysisResult analysis = analyzeAudio (resampled, targetSR);

      if (cancelFlag_.load()) return {};
      if (progress) progress (0.35f);

      // Step 4: Engine-inspired FX chain
      float velocityClamped = juce::jlimit (0.0f, 1.0f, velocityNorm);
      juce::AudioBuffer<float> processed = applyChain (resampled, targetSR, profile, velocityClamped, analysis, rng);

      if (cancelFlag_.load()) return {};
      if (progress) progress (0.80f);

      // Wet/dry blend using intensity parameter
      float intensity = juce::jlimit (0.0f, 1.0f, settings.intensity);
      if (intensity < 0.999f)
      {
          int numCh    = processed.getNumChannels();
          int numSamps = processed.getNumSamples();
          int srcSamps = resampled.getNumSamples();
          for (int ch = 0; ch < numCh; ++ch)
          {
              auto* wet = processed.getWritePointer (ch);
              auto* dry = resampled.getReadPointer (juce::jmin (ch, resampled.getNumChannels() - 1));
              int blendSamps = juce::jmin (numSamps, srcSamps);
              for (int i = 0; i < blendSamps; ++i)
                  wet[i] = dry[i] * (1.0f - intensity) + wet[i] * intensity;
          }
      }

      if (cancelFlag_.load()) return {};

      // Step 5: Output gain compensation (match original input LUFS ±0.5 LU)
      float targetOutputLUFS = juce::jlimit (-40.0f, 0.0f, inputLUFS);
      compensateGain (processed, targetSR, targetOutputLUFS);

      if (progress) progress (0.90f);

      // Step 6: True-peak limiter at -1.0 dBTP
      applyTruePeakLimit (processed, targetSR);

      if (progress) progress (1.0f);
      return processed;
  }

  inline juce::AudioBuffer<float> RebirthPipeline::preview (
      const juce::AudioBuffer<float>& source,
      double sourceSampleRate,
      const RebirthSettings& settings)
  {
      // Preview: process only the first 2 seconds of audio at full quality.
      // Target: <3 seconds wall time on M1.
      constexpr double kPreviewDurationS = 2.0;
      double targetSR = kTargetSampleRate;
      int previewSamps = (int) (sourceSampleRate * kPreviewDurationS);
      int sourceSamps  = source.getNumSamples();

      if (sourceSamps <= previewSamps)
      {
          // Short sample — process entire buffer
          juce::Random rng (42);
          return process (source, sourceSampleRate, settings, 0.7f, rng);
      }

      // Trim to preview length
      juce::AudioBuffer<float> trimmed (source.getNumChannels(), previewSamps);
      for (int ch = 0; ch < source.getNumChannels(); ++ch)
          trimmed.copyFrom (ch, 0, source, ch, 0, previewSamps);

      juce::Random rng (42);
      return process (trimmed, sourceSampleRate, settings, 0.7f, rng);
  }
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add process() and preview() to RebirthPipeline — complete pipeline orchestrator`

---

### Task 5: XOutshine.h Integration

**What:** Modify `Source/Export/XOutshine.h` to: (a) include the new Rebirth headers, (b) add `RebirthSettings rebirth` field to `OutshineSettings`, and (c) wire `RebirthPipeline::process()` into `enhance()` when `settings.rebirth.enabled == true`.

**Why:** Connects the new Rebirth pipeline to the existing Outshine export flow. The Rebirth FX chain replaces the existing velocity amplitude taper and one-pole LP filter in `enhance()` when enabled.

**Files:**
- Modify: `Source/Export/XOutshine.h`

---

- [ ] **Step 5.1 — Add Rebirth includes and RebirthSettings to OutshineSettings**

  In `Source/Export/XOutshine.h` at line 16 (after the existing includes), add:

  ```cpp
  #include "RebirthPipeline.h"
  #include "RebirthProfiles.h"
  ```

  In `OutshineSettings` struct (around line 190), before the closing `};`, add:

  ```cpp
      // Rebirth Mode — Phase 1B
      // When enabled, the velocity amplitude taper and LP filter in enhance() are
      // replaced by the engine-inspired FX chain from RebirthPipeline.
      RebirthSettings rebirth;
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Add RebirthSettings to OutshineSettings and include Rebirth headers in XOutshine.h`

---

- [ ] **Step 5.2 — Wire RebirthPipeline into enhance()**

  In `XOutshine::enhance()`, find the velocity layer loop starting at line 966. The current code generates velocity layers using amplitude scaling and a one-pole LP filter. When `settings_.rebirth.enabled == true`, replace this with calls to `RebirthPipeline::process()`.

  Locate the block that begins:
  ```cpp
  for (int vel = 0; vel < settings_.velocityLayers; ++vel)
  {
      float t = (float) vel / std::max(1, settings_.velocityLayers - 1);
  ```

  Immediately after the `float t = ...` line and before the existing `juce::AudioBuffer<float> shaped(...)` allocation, add a conditional Rebirth branch that uses `continue` after generating layers so the rest of the existing velocity shaping logic is skipped:

  ```cpp
  // Rebirth Mode — Phase 1B: replace velocity amplitude taper with FX chain
  if (settings_.rebirth.enabled)
  {
      RebirthPipeline rebirthPipeline;
      RebirthSettings rebirthCfg = settings_.rebirth;

      // Auto-assign profile from sample category if not explicitly overridden
      // (The RebirthSettings profileId takes precedence — auto-assign is the UI default)
      // The pipeline uses velocityNorm to drive spectral parameter scaling.

      for (int rr = 0; rr < settings_.roundRobin; ++rr)
      {
          float velocityNorm = (settings_.velocityLayers > 1)
              ? (float) vel / (float) (settings_.velocityLayers - 1)
              : 0.7f;

          // Seed RNG per round-robin for variation
          juce::Random rng (static_cast<int64_t> (vel * 1000 + rr));

          juce::AudioBuffer<float> reborn = rebirthPipeline.process (
              original, s.sampleRate, rebirthCfg, velocityNorm, rng, nullptr);

          if (reborn.getNumSamples() == 0) continue;

          // Write the reborn buffer to disk
          juce::AudioFormatManager fmgr2;
          fmgr2.registerBasicFormats();
          auto* wav = fmgr2.findFormatForFileExtension ("wav");
          if (!wav) continue;

          juce::String layerName = juce::String::formatted (
              "%s_vel%02d_rr%02d.wav",
              prog.name.toRawUTF8(), vel + 1, rr + 1);
          juce::File layerFile = enhDir.getChildFile (layerName);

          std::unique_ptr<juce::FileOutputStream> fos (layerFile.createOutputStream());
          if (!fos) continue;

          std::unique_ptr<juce::AudioFormatWriter> writer (
              wav->createWriterFor (fos.get(), s.sampleRate, 2, 24, {}, 0));
          if (writer)
          {
              fos.release();  // writer takes ownership
              writer->writeFromAudioSampleBuffer (reborn, 0, reborn.getNumSamples());
              writer.reset();

              EnhancedLayer layer;
              layer.file     = layerFile;
              layer.filename = layerName;
              layer.velLayer = vel;
              layer.rrIndex  = rr;
              prog.layers.push_back (layer);
          }
      }
      continue;  // Skip the standard amplitude-taper path for this vel layer
  }
  ```

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Wire RebirthPipeline into XOutshine::enhance() — replaces amplitude taper when rebirth.enabled`

---

### Task 6: OutshineAutoMode.h — Enable Rebirth Panel UI

**What:** Modify `Source/UI/Outshine/OutshineAutoMode.h` to replace the dimmed `rebirthTeaser` label with a functional `RebirthPanel` — profile card selector, intensity slider, material mismatch warning, and A/B toggle placeholder.

**Why:** The teaser at line 134 (alpha 0.35) was a placeholder. Phase 1B activates it. The panel exposes `RebirthSettings` to the user. Depends on Task 5 (XOutshine.h must have `RebirthSettings` defined).

**Files:**
- Modify: `Source/UI/Outshine/OutshineAutoMode.h`

---

- [ ] **Step 6.1 — Replace buildRebirthTeaser() with buildRebirthPanel()**

  In `OutshineAutoMode.h`, replace the `buildRebirthTeaser()` method and `juce::Label rebirthTeaser` member with a proper `RebirthPanel` inner class and new state. This step replaces the entire Rebirth section of the file.

  Replace the `buildRebirthTeaser()` private method body (lines 132–142):
  ```cpp
  void buildRebirthTeaser()
  {
      rebirthTeaser.setText("Rebirth Mode \xe2\x80\x94 Phase 1B\n"
                            "Route grains through OPERA \xc2\xb7 OBRIX \xc2\xb7 ONSET",
                            juce::dontSendNotification);
      rebirthTeaser.setFont(GalleryFonts::body(11.0f));
      rebirthTeaser.setColour(juce::Label::textColourId,
                              GalleryColors::get(GalleryColors::textMid()));
      rebirthTeaser.setAlpha(0.35f);
      addAndMakeVisible(rebirthTeaser);
  }
  ```

  With:
  ```cpp
  void buildRebirthPanel()
  {
      // Enable toggle
      rebirthToggle.setButtonText("Rebirth Mode");
      rebirthToggle.setToggleState(false, juce::dontSendNotification);
      rebirthToggle.setColour(juce::ToggleButton::textColourId,
                              GalleryColors::get(GalleryColors::textDark()));
      addAndMakeVisible(rebirthToggle);

      // Profile selector buttons (5 profiles)
      static const char* kProfileNames[] = { "OBRIX", "ONSET", "OWARE", "OPERA", "OVERWASH" };
      static const char* kProfileLabels[] = { "Harmonic\nCharacter", "Percussive\nCrunch",
                                              "Resonant\nBody", "Harmonic\nShimmer", "Deep\nDiffusion" };
      for (int i = 0; i < 5; ++i)
      {
          profileButtons[i] = std::make_unique<juce::TextButton>(kProfileNames[i]);
          profileButtons[i]->setClickingTogglesState(true);
          profileButtons[i]->setRadioGroupId(101);
          profileButtons[i]->setTooltip(kProfileLabels[i]);
          profileButtons[i]->onClick = [this, i] { currentProfileIndex = i; repaint(); };
          addAndMakeVisible(*profileButtons[i]);
      }
      profileButtons[0]->setToggleState(true, juce::dontSendNotification);  // OBRIX default

      // Intensity slider
      intensitySlider.setRange(0.0, 1.0, 0.01);
      intensitySlider.setValue(0.7, juce::dontSendNotification);
      intensitySlider.setSliderStyle(juce::Slider::LinearHorizontal);
      intensitySlider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 40, 18);
      addAndMakeVisible(intensitySlider);

      intensityLabel.setText("Intensity", juce::dontSendNotification);
      intensityLabel.setFont(GalleryFonts::body(11.0f));
      intensityLabel.setColour(juce::Label::textColourId, GalleryColors::get(GalleryColors::textMid()));
      addAndMakeVisible(intensityLabel);

      // Material warning label
      warningLabel.setText("", juce::dontSendNotification);
      warningLabel.setFont(GalleryFonts::body(10.0f));
      warningLabel.setColour(juce::Label::textColourId, juce::Colours::orange);
      addAndMakeVisible(warningLabel);
  }
  ```

  Replace the `juce::Label rebirthTeaser;` member declaration with:
  ```cpp
  juce::ToggleButton                          rebirthToggle;
  std::unique_ptr<juce::TextButton>           profileButtons[5];
  juce::Slider                                intensitySlider;
  juce::Label                                 intensityLabel;
  juce::Label                                 warningLabel;
  int                                         currentProfileIndex = 0;
  ```

  Update `buildRebirthTeaser()` call site in the constructor (line 25) to `buildRebirthPanel()`.

  Update the `rebirthTeaser.setBounds(...)` call in `resized()` (line 102) to lay out the new controls within the `kTeaserH` area:
  ```cpp
  auto rebirthArea = area.removeFromTop(kTeaserH);
  rebirthToggle.setBounds(rebirthArea.removeFromTop(20));
  auto btnRow = rebirthArea.removeFromTop(26);
  int btnW = btnRow.getWidth() / 5;
  for (int i = 0; i < 5; ++i)
      profileButtons[i]->setBounds(btnRow.removeFromLeft(btnW).reduced(2, 0));
  auto sliderRow = rebirthArea.removeFromTop(22);
  intensityLabel.setBounds(sliderRow.removeFromLeft(54));
  intensitySlider.setBounds(sliderRow);
  warningLabel.setBounds(rebirthArea.removeFromTop(16));
  ```

  Update `kTeaserH` from `60` to `100` to accommodate the expanded panel.

  Build verify: `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Commit: `Enable Rebirth panel in OutshineAutoMode — profile selector, intensity slider, warning label`

---

### Task 7: Python CLI Deprecation Notice

**What:** Add a deprecation notice to the Rebirth Mode docstring in `Tools/xoutshine.py`.

**Why:** GATE-02 from the spec. The Python CLI prototype Rebirth Mode (profiles OBESE, OUROBOROS, OPAL, ORIGAMI, OVERDUB) is deprecated as of Phase 1B. Producers using the CLI must be directed to the desktop app. This is a 3-line change — can be done in parallel with Task 6.

**Files:**
- Modify: `Tools/xoutshine.py`

---

- [ ] **Step 7.1 — Add deprecation notice to xoutshine.py Rebirth docstring**

  Find the Rebirth Mode function or class in `Tools/xoutshine.py` (search for `"rebirth"` or `"OBESE"` or `"OUROBOROS"` in the file). Locate the module-level or function-level docstring for the Rebirth Mode section.

  Add the following text at the top of the relevant docstring (or as a module-level comment if there is no class/function docstring):

  ```python
  """
  NOTE: Prototype implementation. Production Rebirth Mode is in the XOlokun desktop app.
  The Python CLI Rebirth profiles (OBESE, OUROBOROS, OPAL, ORIGAMI, OVERDUB) are deprecated
  as of Phase 1B (2026-03-25) and will not receive further updates.
  """
  ```

  Verify: `python3 Tools/xoutshine.py --help` still runs without errors (Python syntax check only).

  Commit: `GATE-02: Deprecate Python CLI Rebirth Mode — direct users to XOlokun desktop app`

---

### Task 8: Tests, CMakeLists.txt, Build Validation, and auval

**What:** Add `Tests/ExportTests/RebirthTests.cpp`, register it in `Tests/CMakeLists.txt`, run a full release build, and validate the AU plugin passes auval.

**Why:** Ensures the entire implementation compiles in isolation (tests instantiate each DSP module without JUCE plugin context) and that the AU plugin still passes system validation after all changes.

**Files:**
- Create: `Tests/ExportTests/RebirthTests.cpp`
- Modify: `Tests/CMakeLists.txt`

---

- [ ] **Step 8.1 — Create Tests/ExportTests/ directory and RebirthTests.cpp**

  Create `Tests/ExportTests/RebirthTests.cpp`:

  ```cpp
  //==============================================================================
  // RebirthTests.cpp — Unit tests for RebirthDSP, RebirthProfiles, and
  // RebirthPipeline. Run with CTest after cmake --build build.
  //
  // These tests do NOT require JUCE's audio plugin infrastructure.
  // They test DSP math and data structure correctness in isolation.
  //==============================================================================

  // Minimal JUCE stubs: include only headers that compile standalone.
  // The Tests/ CMakeLists.txt links against the XOlokun test target which
  // provides JUCE module includes via target_include_directories.
  #include "../../Source/Export/RebirthDSP.h"
  #include "../../Source/Export/RebirthProfiles.h"

  #include <cassert>
  #include <cmath>
  #include <iostream>
  #include <string>

  namespace xolokun {

  //------------------------------------------------------------------------------
  // Test helpers
  //------------------------------------------------------------------------------
  static int gTestsPassed = 0;
  static int gTestsFailed = 0;

  #define ASSERT_TRUE(cond) \
      do { \
          if (!(cond)) { \
              std::cerr << "FAIL: " #cond " (" __FILE__ ":" << __LINE__ << ")\n"; \
              ++gTestsFailed; \
          } else { \
              ++gTestsPassed; \
          } \
      } while (false)

  #define ASSERT_NEAR(a, b, eps) ASSERT_TRUE(std::abs((a) - (b)) <= (eps))

  //------------------------------------------------------------------------------
  // SoftClipGuard tests
  //------------------------------------------------------------------------------
  static void test_softClip()
  {
      // softClip(0) == 0
      ASSERT_NEAR(softClip(0.0f), 0.0f, 1e-5f);

      // softClip is bounded: |output| < 1.0 for any finite input
      ASSERT_TRUE(std::abs(softClip(100.0f)) < 1.0f);
      ASSERT_TRUE(std::abs(softClip(-100.0f)) < 1.0f);

      // softClip is monotonically increasing (odd function)
      ASSERT_TRUE(softClip(0.5f) > softClip(0.3f));
      ASSERT_TRUE(softClip(-0.5f) < softClip(-0.3f));
  }

  //------------------------------------------------------------------------------
  // AllpassDiffuser tests
  //------------------------------------------------------------------------------
  static void test_allpassDiffuser()
  {
      AllpassDiffuser diff;
      diff.prepare(44100.0, 512);

      // A silent input should produce silent output (no self-oscillation)
      std::vector<float> L(512, 0.0f), R(512, 0.0f);
      diff.processBlock(L.data(), R.data(), 512);

      float maxOut = 0.0f;
      for (int i = 0; i < 512; ++i) maxOut = std::max(maxOut, std::abs(L[i]));
      ASSERT_TRUE(maxOut < 1e-10f);

      // An impulse should produce output that decays (allpass property)
      diff.reset();
      L.assign(512, 0.0f);
      L[0] = 1.0f;
      diff.processBlock(L.data(), R.data(), 512);

      // Output should NOT be all zero (allpass diffuses the impulse)
      float energy = 0.0f;
      for (int i = 1; i < 512; ++i) energy += L[i] * L[i];
      ASSERT_TRUE(energy > 0.001f);

      // Energy should be less than 10 (allpass is energy-preserving not amplifying)
      ASSERT_TRUE(energy < 10.0f);
  }

  //------------------------------------------------------------------------------
  // NoiseBurst tests
  //------------------------------------------------------------------------------
  static void test_noiseBurst()
  {
      NoiseBurst burst;
      burst.prepare(44100.0, 512);

      // Without trigger: output should be unchanged (no burst)
      std::vector<float> L(512, 0.0f), R(512, 0.0f);
      burst.processBlock(L.data(), R.data(), 512);
      float energy = 0.0f;
      for (int i = 0; i < 512; ++i) energy += L[i] * L[i];
      ASSERT_NEAR(energy, 0.0f, 1e-10f);

      // With trigger: output should have signal during burst duration
      burst.setBurstLengthMs(5.0f);
      burst.setBurstLevelDb(-20.0f);
      burst.setHPFCutoffHz(1000.0f);
      burst.triggerBurst();
      L.assign(512, 0.0f); R.assign(512, 0.0f);
      burst.processBlock(L.data(), R.data(), 512);

      // There should be some energy in the burst region (first ~220 samples @ 44.1k)
      energy = 0.0f;
      for (int i = 0; i < 220; ++i) energy += L[i] * L[i];
      ASSERT_TRUE(energy > 1e-6f);
  }

  //------------------------------------------------------------------------------
  // RebirthProfiles data integrity tests
  //------------------------------------------------------------------------------
  static void test_profileDataIntegrity()
  {
      // All 5 profiles should be retrievable without crash
      for (int i = 0; i < 5; ++i)
      {
          auto id = static_cast<RebirthProfileID>(i);
          const RebirthProfile& p = getRebirthProfile(id);

          // Profile should have at least one module in the chain
          ASSERT_TRUE(p.chain.size() >= 1);

          // engineName should not be null or empty
          ASSERT_TRUE(p.engineName != nullptr && std::string(p.engineName).size() > 0);

          // recommendedTransientMax should be in [0, 1]
          ASSERT_TRUE(p.recommendedTransientMax >= 0.0f && p.recommendedTransientMax <= 1.0f);

          // tailSeconds should be positive
          ASSERT_TRUE(p.tailSeconds > 0.0f);
      }

      // All profiles must end with WetDryMix as last module
      for (int i = 0; i < 5; ++i)
      {
          auto id = static_cast<RebirthProfileID>(i);
          const RebirthProfile& p = getRebirthProfile(id);
          ASSERT_TRUE(p.chain.back().moduleId == RebirthDSPModuleID::WetDryMix);
      }
  }

  //------------------------------------------------------------------------------
  // resolveVelocityParam tests
  //------------------------------------------------------------------------------
  static void test_resolveVelocityParam()
  {
      DSPModuleConfig cfg;
      cfg.params["drive"] = 0.4f;
      cfg.velocityScale["drive"] = { 0.2f, 0.8f };

      // At vel 0.0: should return v0 = 0.2
      ASSERT_NEAR(resolveVelocityParam(cfg, "drive", 0.0f), 0.2f, 1e-5f);

      // At vel 1.0: should return v1 = 0.8
      ASSERT_NEAR(resolveVelocityParam(cfg, "drive", 1.0f), 0.8f, 1e-5f);

      // At vel 0.5: should return midpoint = 0.5
      ASSERT_NEAR(resolveVelocityParam(cfg, "drive", 0.5f), 0.5f, 1e-5f);

      // Missing key: should return 0.0 (default)
      ASSERT_NEAR(resolveVelocityParam(cfg, "nonexistent", 0.5f), 0.0f, 1e-5f);
  }

  //------------------------------------------------------------------------------
  // autoProfileForCategory tests
  //------------------------------------------------------------------------------
  static void test_autoProfileForCategory()
  {
      ASSERT_TRUE(autoProfileForCategory(SampleCategory::Kick)   == RebirthProfileID::ONSET);
      ASSERT_TRUE(autoProfileForCategory(SampleCategory::Snare)  == RebirthProfileID::ONSET);
      ASSERT_TRUE(autoProfileForCategory(SampleCategory::Bass)   == RebirthProfileID::OBRIX);
      ASSERT_TRUE(autoProfileForCategory(SampleCategory::Vocal)  == RebirthProfileID::OPERA);
      ASSERT_TRUE(autoProfileForCategory(SampleCategory::Pad)    == RebirthProfileID::OVERWASH);
      ASSERT_TRUE(autoProfileForCategory(SampleCategory::Unknown) == RebirthProfileID::OBRIX);
  }

  } // namespace xolokun

  int main()
  {
      using namespace xolokun;
      std::cout << "Running Rebirth DSP tests...\n";

      test_softClip();
      test_allpassDiffuser();
      test_noiseBurst();
      test_profileDataIntegrity();
      test_resolveVelocityParam();
      test_autoProfileForCategory();

      std::cout << "PASSED: " << gTestsPassed << "\n";
      std::cout << "FAILED: " << gTestsFailed << "\n";
      return gTestsFailed > 0 ? 1 : 0;
  }
  ```

  Commit: `Add RebirthTests.cpp — unit tests for DSP modules, profiles, and velocity resolution`

---

- [ ] **Step 8.2 — Register RebirthTests in Tests/CMakeLists.txt**

  Modify `Tests/CMakeLists.txt` to add a second test target for the Rebirth tests. Note: `RebirthTests.cpp` uses `SampleCategory` from `XOutshine.h`, so it needs the full include path. Since `XOutshine.h` depends on JUCE, this test target should be structured similarly to FamilyWaveguideTest but with a note that a JUCE-free subset is validated.

  Replace the contents of `Tests/CMakeLists.txt` with:

  ```cmake
  cmake_minimum_required(VERSION 3.15)
  project(XOlokunTests LANGUAGES CXX)

  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  # --- FamilyWaveguideTest (existing) ---
  add_executable(FamilyWaveguideTest FamilyWaveguideTest.cpp)
  target_include_directories(FamilyWaveguideTest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_compile_options(FamilyWaveguideTest PRIVATE
      $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wpedantic>
  )

  # --- RebirthTests ---
  # Tests for RebirthDSP.h, RebirthProfiles.h (JUCE-free subset).
  # RebirthPipeline.h and RebirthLUFS.h require JUCE audio module linkage
  # and are validated via the main plugin build (cmake --build build).
  # This target validates DSP math and profile data structures in isolation.
  add_executable(RebirthTests ExportTests/RebirthTests.cpp)
  target_include_directories(RebirthTests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
  target_compile_options(RebirthTests PRIVATE
      $<$<CXX_COMPILER_ID:Clang,AppleClang,GNU>:-Wall -Wextra -Wpedantic>
  )

  enable_testing()
  add_test(NAME FamilyWaveguideTest COMMAND FamilyWaveguideTest)
  add_test(NAME RebirthTests       COMMAND RebirthTests)
  ```

  Note: `RebirthTests.cpp` includes `RebirthProfiles.h` which includes `RebirthDSP.h` which includes `<juce_audio_basics/juce_audio_basics.h>`. If a JUCE dependency causes issues in the standalone Tests build, refactor `RebirthTests.cpp` to test only the JUCE-free parts (AllpassDiffuser, NoiseBurst, SoftClipGuard) by defining a minimal JUCE stub. The `SampleCategory` enum used by `autoProfileForCategory` must also be available — include `"../../Source/Export/XOutshine.h"` with appropriate guards or copy the minimal `SampleCategory` enum into the test.

  Build verify (plugin): `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build`

  Build verify (tests): `cmake -B Tests/build -G Ninja -S Tests && cmake --build Tests/build && ctest --test-dir Tests/build --output-on-failure`

  Commit: `Register RebirthTests in Tests/CMakeLists.txt — add to CTest suite`

---

- [ ] **Step 8.3 — Full release build and auval validation**

  Run the complete build and AU validation:

  ```bash
  cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build
  auval -v aumu Xolk XoOx
  ```

  Expected: Build exits 0, auval exits with "AU VALIDATION SUCCEEDED" (0 errors). Warnings are acceptable.

  If auval fails: check that `RebirthPipeline.h` and `RebirthLUFS.h` do not instantiate any JUCE objects at static initialization time (all objects must be created inside methods, not as static/global instances).

  Commit: `Phase 1B complete — Rebirth Mode passes build and auval validation`

---

## Summary of New Files

| File | Type | Purpose |
|------|------|---------|
| `Source/Export/RebirthDSP.h` | Create | AllpassDiffuser, FormantResonator, NoiseBurst, SoftClipGuard |
| `Source/Export/RebirthLUFS.h` | Create | K-weighted LUFS measurement (BS.1770-4) |
| `Source/Export/RebirthProfiles.h` | Create | 5 profile definitions + enums + resolveVelocityParam |
| `Source/Export/RebirthPipeline.h` | Create | Pipeline orchestrator (6-step chain executor) |
| `Tests/ExportTests/RebirthTests.cpp` | Create | DSP + profile unit tests |

## Summary of Modified Files

| File | Change |
|------|--------|
| `Source/Export/XOutshine.h` | Include Rebirth headers; add `RebirthSettings rebirth` to `OutshineSettings`; wire into `enhance()` |
| `Source/UI/Outshine/OutshineAutoMode.h` | Replace teaser label with functional Rebirth panel (profile selector + intensity + warning) |
| `Tools/xoutshine.py` | GATE-02: Deprecation notice in Rebirth Mode docstring |
| `Tests/CMakeLists.txt` | Register `RebirthTests` CTest target |

## Dependency Ordering (Task Dependencies)

```
Task 1 (RebirthDSP.h)
    └─→ Task 3 (RebirthProfiles.h, imports DSP module IDs)
            └─→ Task 4 (RebirthPipeline.h, depends on profiles + DSP)
                    └─→ Task 5 (XOutshine.h integration)
                            └─→ Task 6 (UI enablement)

Task 2 (RebirthLUFS.h)   ← parallel with Tasks 1-3, required by Task 4
Task 7 (Python CLI)       ← parallel, no C++ dependencies
Task 8 (Tests + auval)   ← depends on all above (final gate)
```
