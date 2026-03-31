import SwiftUI

/// Audio + display settings — Sprint 39.
/// All values persist via @AppStorage (UserDefaults).
/// Pro Mode switches MicroscopeView from game language to synth-param language.
struct AudioSettingsView: View {

    // MARK: - Persistent State

    @AppStorage("audio.outputLevel")       private var outputLevel: Double = 0.85
    @AppStorage("audio.reverbMix")         private var reverbMix: Double = 0.25
    @AppStorage("audio.limiterEnabled")    private var limiterEnabled: Bool = true
    @AppStorage("audio.proMode")           private var proMode: Bool = false
    @AppStorage("audio.sampleRate")        private var sampleRateIndex: Int = 0   // 0=44.1k, 1=48k

    // MARK: - UI State

    @State private var showSampleRateConfirm = false
    @State private var pendingSampleRateIndex: Int? = nil

    // Sample rate options
    private let sampleRates = ["44.1 kHz", "48 kHz"]

    var body: some View {
        ZStack {
            DesignTokens.background.ignoresSafeArea()

            ScrollView {
                VStack(spacing: 0) {
                    headerBar

                    VStack(spacing: DesignTokens.spacing8) {
                        levelSection
                        mixSection
                        safeguardSection
                        interfaceSection
                        engineSection
                        aboutSection
                    }
                    .padding(.horizontal, DesignTokens.spacing16)
                    .padding(.bottom, 40)
                }
            }
        }
        .confirmationDialog(
            "Change Sample Rate?",
            isPresented: $showSampleRateConfirm,
            titleVisibility: .visible
        ) {
            Button("Switch to \(pendingSampleRateIndex.map { sampleRates[$0] } ?? "")") {
                if let idx = pendingSampleRateIndex { sampleRateIndex = idx }
            }
            Button("Cancel", role: .cancel) { pendingSampleRateIndex = nil }
        } message: {
            Text("The audio engine will restart. Any unsaved reef changes will be preserved.")
        }
    }

    // MARK: - Header

    private var headerBar: some View {
        HStack {
            Text("AUDIO SETTINGS")
                .font(DesignTokens.mono(11))
                .tracking(2)
                .foregroundColor(.white.opacity(0.35))
            Spacer()
            // Version badge
            Text("v\(AppConstants.version)")
                .font(DesignTokens.mono(9))
                .foregroundColor(.white.opacity(0.15))
        }
        .padding(.horizontal, DesignTokens.spacing16)
        .padding(.vertical, DesignTokens.spacing16)
    }

    // MARK: - Level Section

    private var levelSection: some View {
        SettingsSection(title: "OUTPUT") {
            VStack(spacing: DesignTokens.spacing12) {
                SettingsSliderRow(
                    label: "Output Level",
                    value: $outputLevel,
                    displayValue: percentString(outputLevel),
                    accentColor: DesignTokens.reefJade
                )
            }
        }
    }

    // MARK: - Mix Section

    private var mixSection: some View {
        SettingsSection(title: "MASTER FX") {
            VStack(spacing: DesignTokens.spacing12) {
                SettingsSliderRow(
                    label: "Master Reverb",
                    value: $reverbMix,
                    displayValue: percentString(reverbMix),
                    accentColor: DesignTokens.effectColor
                )
            }
        }
    }

    // MARK: - Safeguard Section

    private var safeguardSection: some View {
        SettingsSection(title: "PROTECTION") {
            VStack(spacing: DesignTokens.spacing4) {
                SettingsToggleRow(
                    label: "Limiter",
                    sublabel: "Prevents clipping on loud moments",
                    isOn: $limiterEnabled,
                    accentColor: DesignTokens.xoGold
                )
            }
        }
    }

    // MARK: - Interface Section

    private var interfaceSection: some View {
        SettingsSection(title: "INTERFACE") {
            VStack(spacing: DesignTokens.spacing4) {
                SettingsToggleRow(
                    label: "Pro Mode",
                    sublabel: "Shows synth parameters in Microscope View",
                    isOn: $proMode,
                    accentColor: DesignTokens.deepAccent
                )

                if proMode {
                    HStack(spacing: DesignTokens.spacing8) {
                        Image(systemName: "info.circle")
                            .font(.system(size: 10))
                            .foregroundColor(DesignTokens.deepAccent.opacity(0.5))
                        Text("Specimen stats now show synth params: Cutoff, Resonance, Attack, Decay, etc.")
                            .font(DesignTokens.body(10))
                            .foregroundColor(.white.opacity(0.3))
                    }
                    .padding(.top, DesignTokens.spacing4)
                    .transition(.opacity.combined(with: .move(edge: .top)))
                }
            }
            .animation(.easeInOut(duration: 0.2), value: proMode)
        }
    }

    // MARK: - Engine Section

    private var engineSection: some View {
        SettingsSection(title: "ENGINE") {
            VStack(spacing: DesignTokens.spacing8) {
                // Label row
                HStack {
                    Text("Sample Rate")
                        .font(DesignTokens.bodyMedium(13))
                        .foregroundColor(.white.opacity(0.8))
                    Spacer()
                    Text(sampleRates[sampleRateIndex])
                        .font(DesignTokens.mono(12))
                        .foregroundColor(DesignTokens.reefJade)
                }

                // Segmented picker
                Picker("Sample Rate", selection: Binding(
                    get: { sampleRateIndex },
                    set: { newIndex in
                        if newIndex != sampleRateIndex {
                            pendingSampleRateIndex = newIndex
                            showSampleRateConfirm = true
                        }
                    }
                )) {
                    ForEach(0..<sampleRates.count, id: \.self) { idx in
                        Text(sampleRates[idx]).tag(idx)
                    }
                }
                .pickerStyle(.segmented)

                Text("Restart required to apply. 44.1 kHz is standard for music production.")
                    .font(DesignTokens.body(10))
                    .foregroundColor(.white.opacity(0.2))
            }
        }
    }

    // MARK: - About Section

    private var aboutSection: some View {
        SettingsSection(title: "ABOUT") {
            VStack(spacing: DesignTokens.spacing8) {
                aboutRow("App",       value: AppConstants.appName)
                aboutRow("Version",   value: "\(AppConstants.version) (\(AppConstants.build))")
                aboutRow("Developer", value: AppConstants.developer)
                aboutRow("Tagline",   value: AppConstants.tagline)

                if let url = URL(string: AppConstants.website) {
                    Link(destination: url) {
                        HStack {
                            Text("Website")
                                .font(DesignTokens.bodyMedium(12))
                                .foregroundColor(.white.opacity(0.5))
                            Spacer()
                            Text(AppConstants.website)
                                .font(DesignTokens.mono(10))
                                .foregroundColor(DesignTokens.reefJade.opacity(0.7))
                            Image(systemName: "arrow.up.right")
                                .font(.system(size: 9))
                                .foregroundColor(DesignTokens.reefJade.opacity(0.4))
                        }
                    }
                }
            }
        }
    }

    private func aboutRow(_ label: String, value: String) -> some View {
        HStack {
            Text(label)
                .font(DesignTokens.bodyMedium(12))
                .foregroundColor(.white.opacity(0.35))
            Spacer()
            Text(value)
                .font(DesignTokens.mono(11))
                .foregroundColor(.white.opacity(0.5))
        }
    }

    // MARK: - Helpers

    private func percentString(_ value: Double) -> String {
        "\(Int(value * 100))%"
    }
}

// MARK: - Supporting Views

/// Reusable dark section container matching app panel aesthetic.
private struct SettingsSection<Content: View>: View {
    let title: String
    @ViewBuilder let content: () -> Content

    var body: some View {
        VStack(alignment: .leading, spacing: DesignTokens.spacing12) {
            Text(title)
                .font(DesignTokens.mono(9))
                .tracking(1.5)
                .foregroundColor(.white.opacity(0.2))

            VStack(spacing: 0) {
                content()
            }
            .padding(DesignTokens.spacing16)
            .background(
                RoundedRectangle(cornerRadius: 10)
                    .fill(DesignTokens.panelBackground)
                    .overlay(
                        RoundedRectangle(cornerRadius: 10)
                            .stroke(Color.white.opacity(0.05), lineWidth: 1)
                    )
            )
        }
        .padding(.top, DesignTokens.spacing8)
    }
}

/// Slider row with label, live readout, and accent fill color.
private struct SettingsSliderRow: View {
    let label: String
    @Binding var value: Double
    let displayValue: String
    let accentColor: Color

    var body: some View {
        VStack(spacing: DesignTokens.spacing6) {
            HStack {
                Text(label)
                    .font(DesignTokens.bodyMedium(13))
                    .foregroundColor(.white.opacity(0.8))
                Spacer()
                Text(displayValue)
                    .font(DesignTokens.mono(12))
                    .foregroundColor(accentColor)
                    .monospacedDigit()
                    .animation(.none, value: displayValue)
            }

            Slider(value: $value, in: 0...1, step: 0.01)
                .tint(accentColor)
                .onChange(of: value) { _ in
                    HapticEngine.sliderTick()
                }
        }
    }
}

/// Toggle row with label + sublabel, accent color tint.
private struct SettingsToggleRow: View {
    let label: String
    let sublabel: String
    @Binding var isOn: Bool
    let accentColor: Color

    var body: some View {
        Toggle(isOn: $isOn) {
            VStack(alignment: .leading, spacing: 2) {
                Text(label)
                    .font(DesignTokens.bodyMedium(13))
                    .foregroundColor(.white.opacity(0.85))
                Text(sublabel)
                    .font(DesignTokens.body(10))
                    .foregroundColor(.white.opacity(0.3))
            }
        }
        .tint(accentColor)
        .onChange(of: isOn) { _ in
            HapticEngine.sliderTick()
        }
    }
}

// MARK: - HapticEngine Helper (bridge from toggle/slider to existing API)
// AudioSettingsView uses the public `sliderTick()` method directly.
// Toggle haptic is sourced from `UISelectionFeedbackGenerator` via the public method below.

// MARK: - Preview

#if DEBUG
#Preview {
    AudioSettingsView()
        .background(DesignTokens.background.ignoresSafeArea())
}
#endif
