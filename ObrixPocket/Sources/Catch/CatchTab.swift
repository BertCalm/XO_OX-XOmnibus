import SwiftUI

// MARK: - CatchTab

/// The Catch tab — circular radar showing nearby wild specimens.
///
/// Layout:
///   - Biome indicator strip (top)
///   - Radar view: concentric rings, user dot at center, specimen pips at bearing + distance
///   - Reef Proximity toggle (bottom strip)
///
/// Specimen pips are tappable when the specimen is within 50m (or always in Reef Proximity mode).
/// Tapping opens CatchScreen as a sheet.
struct CatchTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @StateObject private var biomeDetector = BiomeDetector()
    @StateObject private var spectralCapture = SpectralCapture()
    @State private var spawnManager: SpawnManager?
    @State private var showingCatch: WildSpecimen?
    @State private var catchPhase: CatchPhase = .idle

    var body: some View {
        ZStack {
            Color(hex: "0E0E10").ignoresSafeArea()

            VStack(spacing: 16) {
                biomeStrip
                radarView
                reefProximityStrip
            }
        }
        .onAppear(perform: onAppear)
        .sheet(item: $showingCatch) { wild in
            CatchScreen(specimen: wild, spectralCapture: spectralCapture, phase: $catchPhase)
        }
    }

    // MARK: - Sub-views

    private var biomeStrip: some View {
        HStack(spacing: 6) {
            Circle()
                .fill(biomeColor(for: biomeDetector.currentBiome))
                .frame(width: 8, height: 8)
            Text(biomeDetector.currentBiome.displayName.uppercased())
                .font(.custom("SpaceGrotesk-Bold", size: 11))
                .tracking(1)
                .foregroundColor(.white)
            Spacer()
            if !biomeDetector.locationAuthorized {
                Button("Enable Location") {
                    biomeDetector.requestAuthorization()
                }
                .font(.custom("Inter-Medium", size: 11))
                .foregroundColor(Color(hex: "1E8B7E"))
            }
        }
        .padding(.horizontal, 20)
        .padding(.top, 12)
    }

    private var radarView: some View {
        GeometryReader { geometry in
            let radarSize = min(geometry.size.width, geometry.size.height) * 0.85

            ZStack {
                // Radar rings (3 distance bands)
                ForEach(1..<4) { ring in
                    Circle()
                        .stroke(Color.white.opacity(0.06), lineWidth: 1)
                        .frame(
                            width:  radarSize * CGFloat(ring) / 3,
                            height: radarSize * CGFloat(ring) / 3
                        )
                }

                // Player position — Reef Jade dot
                Circle()
                    .fill(Color(hex: "1E8B7E"))
                    .frame(width: 12, height: 12)

                // Wild specimen pips
                if let manager = spawnManager {
                    ForEach(manager.wildSpecimens) { wild in
                        SpecimenPip(specimen: wild, radarSize: radarSize)
                            .onTapGesture { handlePipTap(wild) }
                    }
                }
            }
            .frame(width: radarSize, height: radarSize)
            .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
    }

    private var reefProximityStrip: some View {
        HStack(spacing: 8) {
            Image(systemName: "house.fill")
                .font(.system(size: 12))
                .foregroundColor(Color(hex: "7A7876"))
            Toggle("Reef Proximity", isOn: Binding(
                get: { biomeDetector.reefProximityEnabled },
                set: { enabled in
                    if enabled {
                        // Use last known location as home, or a neutral coordinate if unavailable
                        let home = biomeDetector.lastLocation ?? CLLocationCoordinate2D(latitude: 0, longitude: 0)
                        biomeDetector.enableReefProximity(home: home)
                    } else {
                        biomeDetector.disableReefProximity()
                    }
                }
            ))
            .font(.custom("Inter-Regular", size: 12))
            .foregroundColor(Color(hex: "7A7876"))
            .tint(Color(hex: "1E8B7E"))
        }
        .padding(.horizontal, 20)
        .padding(.bottom, 12)
    }

    // MARK: - Actions

    private func onAppear() {
        if spawnManager == nil {
            spawnManager = SpawnManager(biomeDetector: biomeDetector)
        }
        // Prune first so we don't snapshot stale specimens
        spawnManager?.pruneExpired()
        spawnManager?.checkDailyDrift()
        spawnManager?.checkLoginMilestone()
        spawnManager?.checkTimeOfDay()
        biomeDetector.requestAuthorization()
    }

    private func handlePipTap(_ wild: WildSpecimen) {
        // Reef Proximity mode: all specimens are catchable regardless of distance
        let catchable = biomeDetector.reefProximityEnabled || wild.distance < 50
        guard catchable else { return }
        catchPhase = .singing
        showingCatch = wild
    }

    // MARK: - Helpers

    private func biomeColor(for biome: Biome) -> Color {
        switch biome {
        case .coastal:     return Color(hex: "00B4A0")
        case .forest:      return Color(hex: "228B22")
        case .urban:       return Color(hex: "778899")
        case .elevation:   return Color(hex: "A8D8EA")
        case .nocturnal:   return Color(hex: "2D0A4E")
        case .storm:       return Color(hex: "FF6B6B")
        case .liminal:     return Color(hex: "8A9BA8")
        case .convergence: return Color(hex: "E9C46A")
        }
    }
}

// MARK: - SpecimenPip

/// A pip on the radar representing one wild specimen at its bearing + distance.
private struct SpecimenPip: View {
    let specimen: WildSpecimen
    let radarSize: CGFloat

    var body: some View {
        let normalizedDist = CGFloat(min(specimen.distance / 500.0, 1.0)) // 500m = radar edge
        let x = CGFloat(cos(specimen.direction)) * normalizedDist * radarSize / 2
        let y = CGFloat(sin(specimen.direction)) * normalizedDist * radarSize / 2

        Circle()
            .fill(categoryColor)
            .frame(width: pipSize, height: pipSize)
            .shadow(color: categoryColor.opacity(0.5), radius: 4)
            .offset(x: x, y: y)
    }

    private var pipSize: CGFloat {
        switch specimen.rarity {
        case .legendary: return 16
        case .rare:      return 12
        case .uncommon:  return 10
        case .common:    return 8
        }
    }

    private var categoryColor: Color {
        switch specimen.category {
        case .source:    return Color(hex: "3380FF") // Blue  — Shells
        case .processor: return Color(hex: "FF4D4D") // Red   — Coral
        case .modulator: return Color(hex: "4DCC4D") // Green — Currents
        case .effect:    return Color(hex: "B34DFF") // Purple — Tide Pools
        }
    }
}

// MARK: - CatchPhase

/// Stages of a catch encounter.
enum CatchPhase {
    case idle         // Sheet just opened, nothing started
    case singing      // Specimen plays its generative phrase; player interacts with touch strip
    case capturing    // SpectralCapture running (5-second ambient mic snapshot)
    case harmonizing  // Player achieved a harmonic lock during singing phase
    case caught       // Success — brief celebration before dismiss
}

// MARK: - CatchScreen

/// Full-screen sheet for a catch encounter: musical duet + spectral DNA capture.
///
/// Flow: singing → [harmonizing] → capturing → caught → auto-dismiss after 1.5s.
struct CatchScreen: View {
    let specimen: WildSpecimen
    @ObservedObject var spectralCapture: SpectralCapture
    @Binding var phase: CatchPhase
    @Environment(\.dismiss) private var dismiss

    var body: some View {
        ZStack {
            Color(hex: "0A0A0F").ignoresSafeArea()

            VStack(spacing: 24) {
                Spacer()

                specimenCreature
                statusLabel

                if spectralCapture.isCapturing {
                    captureProgressBar
                }

                if phase == .singing || phase == .harmonizing {
                    touchStrip
                }

                catchButton
                Spacer()
            }
        }
    }

    // MARK: - Sub-views

    private var specimenCreature: some View {
        ZStack {
            // Ambient glow ring — pulses with biome color during singing/harmonizing
            Circle()
                .fill(categoryColor.opacity(0.12))
                .frame(width: 200, height: 200)

            // Placeholder glyph — Phase 1 will render .xogenome pixel art here
            Text(String(specimen.subtype.prefix(3)).uppercased())
                .font(.custom("JetBrainsMono-Bold", size: 36))
                .foregroundColor(categoryColor)

            // Rarity ring
            Circle()
                .stroke(categoryColor.opacity(0.6), lineWidth: rarityRingWidth)
                .frame(width: 180, height: 180)
        }
    }

    private var statusLabel: some View {
        VStack(spacing: 4) {
            Text(phaseHeadline)
                .font(.custom("SpaceGrotesk-Bold", size: 16))
                .foregroundColor(.white)
            Text(phaseSubtext)
                .font(.custom("Inter-Regular", size: 12))
                .foregroundColor(.white.opacity(0.5))
                .multilineTextAlignment(.center)
                .padding(.horizontal, 32)
        }
    }

    private var captureProgressBar: some View {
        VStack(spacing: 6) {
            ProgressView(value: spectralCapture.captureProgress)
                .tint(categoryColor)
                .padding(.horizontal, 40)
            Text("Capturing spectral DNA... \(Int(spectralCapture.captureProgress * 100))%")
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(.white.opacity(0.4))
        }
    }

    private var touchStrip: some View {
        TouchStrip { velocity in
            // Any touch during singing counts as a harmonic attempt
            if phase == .singing {
                phase = .harmonizing
            }
        }
        .frame(height: 60)
        .padding(.horizontal, 20)
    }

    private var catchButton: some View {
        Button(action: attemptCatch) {
            Text(catchButtonLabel)
                .font(.custom("SpaceGrotesk-Bold", size: 16))
                .tracking(2)
                .foregroundColor(.white)
                .frame(maxWidth: .infinity)
                .frame(height: 50)
                .background(
                    RoundedRectangle(cornerRadius: 25)
                        .fill(phase == .caught ? Color(hex: "1E8B7E") : categoryColor)
                )
        }
        .disabled(phase == .capturing || phase == .caught)
        .padding(.horizontal, 40)
    }

    // MARK: - Catch Action

    private func attemptCatch() {
        guard phase == .singing || phase == .harmonizing || phase == .idle else { return }
        phase = .capturing
        spectralCapture.startCapture { profile in
            // TODO Phase 1: Construct a Specimen from wild + profile, call ReefStore.addSpecimen()
            phase = .caught
            DispatchQueue.main.asyncAfter(deadline: .now() + 1.5) {
                dismiss()
            }
        }
    }

    // MARK: - Helpers

    private var catchButtonLabel: String {
        switch phase {
        case .idle, .singing, .harmonizing: return "CATCH"
        case .capturing:                    return "LISTENING..."
        case .caught:                       return "CAUGHT!"
        }
    }

    private var phaseHeadline: String {
        switch phase {
        case .idle:        return "A wild \(specimen.subtype) appeared!"
        case .singing:     return "The specimen is singing..."
        case .capturing:   return "Capturing spectral DNA"
        case .harmonizing: return "Harmonic lock!"
        case .caught:      return "Specimen caught!"
        }
    }

    private var phaseSubtext: String {
        switch phase {
        case .idle:        return "Tap CATCH to begin the encounter."
        case .singing:     return "Play along on the touch strip to improve the catch."
        case .capturing:   return "Hold still — the reef is listening."
        case .harmonizing: return "Keep playing — lock is holding."
        case .caught:      return "Added to your collection."
        }
    }

    private var rarityRingWidth: CGFloat {
        switch specimen.rarity {
        case .legendary: return 4
        case .rare:      return 3
        case .uncommon:  return 2
        case .common:    return 1
        }
    }

    private var categoryColor: Color {
        switch specimen.category {
        case .source:    return Color(hex: "3380FF")
        case .processor: return Color(hex: "FF4D4D")
        case .modulator: return Color(hex: "4DCC4D")
        case .effect:    return Color(hex: "B34DFF")
        }
    }
}

// MARK: - TouchStrip

/// Horizontal touch strip for the musical catch duet.
/// The player drags left/right to play the specimen's generative phrase in key.
/// Velocity (0–1) is reported via `onPlay`.
struct TouchStrip: View {
    let onPlay: (Float) -> Void

    var body: some View {
        GeometryReader { geometry in
            ZStack {
                RoundedRectangle(cornerRadius: 8)
                    .fill(Color.white.opacity(0.04))
                RoundedRectangle(cornerRadius: 8)
                    .stroke(Color.white.opacity(0.1), lineWidth: 1)
                Text("PLAY ALONG")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .tracking(2)
                    .foregroundColor(.white.opacity(0.2))
            }
            .gesture(
                DragGesture(minimumDistance: 0)
                    .onChanged { value in
                        let velocity = Float(max(0, min(1, value.location.x / geometry.size.width)))
                        onPlay(velocity)
                    }
            )
        }
    }
}

// MARK: - CLLocationCoordinate2D import

import CoreLocation
