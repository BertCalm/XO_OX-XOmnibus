import SwiftUI
import MapKit

// MARK: - CatchTab

/// The Catch tab — MapKit map showing nearby wild specimens as pins.
///
/// Layout:
///   - Biome indicator strip (top)
///   - Map view: real MapKit map, user dot at current location, specimen pins at bearing + distance
///   - Reef Proximity toggle (bottom strip)
///
/// Specimen pins are tappable when the specimen is within 50m (or always in Reef Proximity mode).
/// Tapping opens CatchScreen as a sheet.
struct CatchTab: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @StateObject private var biomeDetector = BiomeDetector()
    @StateObject private var spectralCapture = SpectralCapture()
    @StateObject private var weatherService = WeatherService()
    @State private var spawnManager: SpawnManager?
    @State private var wildSpecimens: [WildSpecimen] = []
    @State private var showingCatch: WildSpecimen?
    @State private var catchPhase: CatchPhase = .intro
    @State private var mapRegion = MKCoordinateRegion(
        center: CLLocationCoordinate2D(latitude: 37.7749, longitude: -122.4194),
        span: MKCoordinateSpan(latitudeDelta: 0.01, longitudeDelta: 0.01)
    )

    var body: some View {
        ZStack {
            Color(hex: "0E0E10").ignoresSafeArea()

            VStack(spacing: 16) {
                biomeStrip
                journeyHintBanner
                mapView
                reefProximityStrip
            }
        }
        .onAppear(perform: onAppear)
        .onChange(of: weatherService.isStormActive) { isStorm in
            biomeDetector.updateStormBiome(isStorm: isStorm)
        }
        .sheet(item: $showingCatch) { wild in
            CatchScreen(
                specimen: wild,
                spectralCapture: spectralCapture,
                phase: $catchPhase,
                catchLocation: biomeDetector.lastLocation,
                catchWeather: weatherService.bestAvailable
            )
            .environmentObject(reefStore)
            .environmentObject(firstLaunchManager)
        }
    }

    // MARK: - Sub-views

    /// Shown only during the guided tutorial journey. Disappears after all 8 specimens are caught.
    @ViewBuilder
    private var journeyHintBanner: some View {
        if !firstLaunchManager.isJourneyComplete,
           let subtype = firstLaunchManager.nextJourneySubtype,
           // Only show the banner for wild-catch steps (step 2+); steps 0-1 are auto-placed
           firstLaunchManager.journeyStep >= 2 {
            let name = SpecimenCatalog.entry(for: subtype)?.creatureName ?? subtype
            let category = firstLaunchManager.nextJourneyCategory
            HStack(spacing: 6) {
                Image(systemName: "sparkle")
                    .foregroundColor(Color(hex: "E9C46A"))
                Text("Look for \(name)")
                    .font(.custom("Inter-Medium", size: 12))
                    .foregroundColor(Color(hex: "E9C46A"))
                Text("— \(categoryLabel(category))")
                    .font(.custom("Inter-Regular", size: 11))
                    .foregroundColor(.white.opacity(0.4))
                Spacer()
            }
            .padding(.horizontal, 20)
            .padding(.vertical, 6)
        }
    }

    private func categoryLabel(_ category: SpecimenCategory?) -> String {
        switch category {
        case .source:    return "a sound source"
        case .processor: return "a sound shaper"
        case .modulator: return "a modulator"
        case .effect:    return "an effect"
        case .none:      return ""
        }
    }

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

    private var mapView: some View {
        Group {
            if !biomeDetector.locationAuthorized {
                VStack(spacing: 12) {
                    Image(systemName: "location.slash")
                        .font(.system(size: 36))
                        .foregroundColor(.white.opacity(0.3))
                    Text("Location access needed")
                        .font(.custom("SpaceGrotesk-Bold", size: 16))
                        .foregroundColor(.white)
                    Text("OBRIX Pocket uses your location to spawn specimens nearby.")
                        .font(.custom("Inter-Regular", size: 12))
                        .foregroundColor(.white.opacity(0.5))
                        .multilineTextAlignment(.center)
                        .padding(.horizontal, 40)

                    Button("Enable Location") {
                        biomeDetector.requestAuthorization()
                    }
                    .font(.custom("SpaceGrotesk-Bold", size: 14))
                    .foregroundColor(Color(hex: "1E8B7E"))

                    // Also offer Reef Proximity as alternative
                    Button("Use Reef Proximity Instead") {
                        let home = CLLocationCoordinate2D(latitude: 37.7749, longitude: -122.4194)
                        biomeDetector.enableReefProximity(home: home)
                    }
                    .font(.custom("Inter-Regular", size: 12))
                    .foregroundColor(.white.opacity(0.4))
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(Color(hex: "0E0E10"))
            } else {
                Map(
                    coordinateRegion: $mapRegion,
                    showsUserLocation: true,
                    annotationItems: wildSpecimens
                ) { specimen in
                    MapAnnotation(coordinate: specimenCoordinate(for: specimen)) {
                        SpecimenMapPin(specimen: specimen)
                            .onTapGesture { handlePipTap(specimen) }
                    }
                }
                .clipShape(RoundedRectangle(cornerRadius: 16))
                .overlay(
                    RoundedRectangle(cornerRadius: 16)
                        .stroke(Color(hex: "1E8B7E").opacity(0.2), lineWidth: 1)
                )
                .padding(.horizontal, 16)
            }
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
                        // Center map on the fixed home location
                        mapRegion = MKCoordinateRegion(
                            center: home,
                            span: MKCoordinateSpan(latitudeDelta: 0.008, longitudeDelta: 0.008)
                        )
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

        // Journey override: force the scripted next specimen during the tutorial
        if !firstLaunchManager.isJourneyComplete {
            spawnManager?.forcedNextSubtype = firstLaunchManager.nextJourneySubtype
            spawnManager?.forcedNextCategory = firstLaunchManager.nextJourneyCategory
        } else {
            spawnManager?.forcedNextSubtype = nil
            spawnManager?.forcedNextCategory = nil
        }

        // Wire WeatherService into biomeDetector so storm state is available
        biomeDetector.weatherService = weatherService

        // Wire exploration bonus and map region update on every location update.
        // Capture mapRegion as a binding-compatible setter on the main queue.
        biomeDetector.onLocationUpdate = { [weak spawnManager] coord in
            spawnManager?.checkExplorationBonus(at: coord)
            // Pan map to follow the user (skip when Reef Proximity holds the map fixed)
            DispatchQueue.main.async {
                guard !biomeDetector.reefProximityEnabled else { return }
                mapRegion = MKCoordinateRegion(
                    center: coord,
                    span: MKCoordinateSpan(latitudeDelta: 0.008, longitudeDelta: 0.008)
                )
            }
        }

        // Prune first so we don't snapshot stale specimens
        spawnManager?.pruneExpired()
        spawnManager?.checkDailyDrift()
        spawnManager?.checkLoginMilestone()
        spawnManager?.checkTimeOfDay()
        // Snapshot wildSpecimens into @State so SwiftUI re-renders the map
        wildSpecimens = spawnManager?.wildSpecimens ?? []
        biomeDetector.requestAuthorization()

        // Fetch weather for current location if already available
        if let loc = biomeDetector.lastLocation {
            Task { await weatherService.fetchWeather(at: loc) }
            // Center map on actual user position immediately
            mapRegion = MKCoordinateRegion(
                center: loc,
                span: MKCoordinateSpan(latitudeDelta: 0.008, longitudeDelta: 0.008)
            )
        }
    }

    private func handlePipTap(_ wild: WildSpecimen) {
        // Reef Proximity mode: all specimens are catchable regardless of distance
        let catchable = biomeDetector.reefProximityEnabled || wild.distance < 50
        guard catchable else { return }
        catchPhase = .intro
        showingCatch = wild
    }

    // MARK: - Coordinate Conversion

    /// Converts a WildSpecimen's bearing + distance to a real CLLocationCoordinate2D
    /// relative to the user's (or home) position using spherical-earth haversine offsets.
    private func specimenCoordinate(for specimen: WildSpecimen) -> CLLocationCoordinate2D {
        guard let userLoc = biomeDetector.lastLocation else {
            return CLLocationCoordinate2D(latitude: 0, longitude: 0)
        }
        let earthRadius: Double = 6_371_000 // meters
        let lat1 = userLoc.latitude * .pi / 180
        let lon1 = userLoc.longitude * .pi / 180
        let bearing = specimen.direction
        let d = specimen.distance / earthRadius

        let lat2 = asin(sin(lat1) * cos(d) + cos(lat1) * sin(d) * cos(bearing))
        let lon2 = lon1 + atan2(sin(bearing) * sin(d) * cos(lat1),
                                cos(d) - sin(lat1) * sin(lat2))

        return CLLocationCoordinate2D(
            latitude:  lat2 * 180 / .pi,
            longitude: lon2 * 180 / .pi
        )
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

// MARK: - SpecimenMapPin

/// A map annotation pin representing one wild specimen at its real-world coordinate.
private struct SpecimenMapPin: View {
    let specimen: WildSpecimen

    var body: some View {
        ZStack {
            // Outer glow
            Circle()
                .fill(categoryColor.opacity(0.3))
                .frame(width: pinSize + 8, height: pinSize + 8)

            // Pin body
            Circle()
                .fill(categoryColor)
                .frame(width: pinSize, height: pinSize)

            // Rarity ring
            Circle()
                .stroke(Color.white.opacity(0.6), lineWidth: specimen.rarity == .legendary ? 2 : 1)
                .frame(width: pinSize, height: pinSize)
        }
    }

    private var pinSize: CGFloat {
        switch specimen.rarity {
        case .legendary: return 24
        case .rare:      return 20
        case .uncommon:  return 16
        case .common:    return 14
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

enum CatchPhase {
    case intro       // Specimen appeared, waiting to start
    case watching    // Pattern is playing — watch the sequence
    case playing     // Player's turn — repeat the pattern
    case success     // Pattern matched — catching!
    case caught      // Added to reef
    case escaped     // Failed the pattern — specimen fled
}

// MARK: - CatchScreen (Pattern Match Mini-Game)

/// Musical Simon Says catch challenge.
/// Specimen plays a note sequence → player repeats on 4 pitch buttons.
/// Difficulty scales with rarity.
struct CatchScreen: View {
    let specimen: WildSpecimen
    @ObservedObject var spectralCapture: SpectralCapture
    @Binding var phase: CatchPhase
    let catchLocation: CLLocationCoordinate2D?
    let catchWeather: WeatherSnapshot
    @Environment(\.dismiss) private var dismiss
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager

    @State private var pattern: [Int] = []
    @State private var playerInput: [Int] = []
    @State private var currentPlaybackIndex = -1
    @State private var showReefFullAlert = false

    // Multi-round tracking
    @State private var currentRound = 0
    @State private var roundResults: [Bool] = [] // true = round won, false = round lost
    @State private var roundFeedback: RoundFeedback = .none

    enum RoundFeedback { case none, correct, wrong }

    // Notes per round (pattern length) — scales with rarity
    private var notesPerRound: Int {
        switch specimen.rarity {
        case .common: return 3; case .uncommon: return 4
        case .rare: return 4; case .legendary: return 5
        }
    }

    // Total rounds and required wins
    private var totalRounds: Int {
        switch specimen.rarity {
        case .common: return 5; case .uncommon: return 5
        case .rare: return 7; case .legendary: return 7
        }
    }

    private var requiredWins: Int {
        switch specimen.rarity {
        case .common: return 3; case .uncommon: return 3
        case .rare: return 5; case .legendary: return 5
        }
    }

    private var roundsWon: Int { roundResults.filter { $0 }.count }
    private var roundsLost: Int { roundResults.filter { !$0 }.count }
    private var roundsPlayed: Int { roundResults.count }

    // Can still win? Can still lose?
    private var canStillWin: Bool { roundsWon + (totalRounds - roundsPlayed) >= requiredWins }
    private var alreadyWon: Bool { roundsWon >= requiredWins }
    private var alreadyLost: Bool { !canStillWin }

    private var buttonColors: [Color] {
        [catColor.opacity(0.5), catColor.opacity(0.7), catColor.opacity(0.85), catColor]
    }

    var body: some View {
        ZStack {
            Color(hex: "0A0A0F").ignoresSafeArea()

            VStack(spacing: 20) {
                Spacer()
                specimenHeader
                statusText
                if phase != .intro { scoreIndicator }
                pitchButtonGrid.padding(.horizontal, 40)
                actionButton
                Spacer()
            }
        }
        .onAppear { phase = .intro }
        .alert("Reef Full", isPresented: $showReefFullAlert) {
            Button("OK") { dismiss() }
        } message: {
            Text("Your reef has no empty slots. Release a specimen to make room.")
        }
    }

    private var specimenHeader: some View {
        VStack(spacing: 8) {
            ZStack {
                Circle()
                    .fill(catColor.opacity(0.12))
                    .frame(width: 140, height: 140)
                let cID = SpecimenCatalog.catalogSubtypeID(from: specimen.subtype)
                SpecimenSprite(subtype: cID, category: specimen.category, size: 70)
                Circle()
                    .stroke(catColor.opacity(0.5), lineWidth: specimen.rarity == .legendary ? 4 : (specimen.rarity == .rare ? 3 : (specimen.rarity == .uncommon ? 2 : 1)))
                    .frame(width: 130, height: 130)
            }
            let cID = SpecimenCatalog.catalogSubtypeID(from: specimen.subtype)
            Text(SpecimenCatalog.entry(for: cID)?.creatureName ?? specimen.subtype)
                .font(.custom("SpaceGrotesk-Bold", size: 18))
                .foregroundColor(.white)
            Text(specimen.rarity.rawValue.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(Color(hex: "E9C46A"))
            if phase == .intro, let entry = SpecimenCatalog.entry(for: cID) {
                Text(entry.personalityLine)
                    .font(.custom("Inter-Regular", size: 11))
                    .foregroundColor(.white.opacity(0.4))
                    .italic()
                    .multilineTextAlignment(.center)
                    .padding(.horizontal, 32)
            }
        }
    }

    private var statusText: some View {
        VStack(spacing: 4) {
            Text(headline)
                .font(.custom("SpaceGrotesk-Bold", size: 16))
                .foregroundColor(.white)
            Text(subtext)
                .font(.custom("Inter-Regular", size: 12))
                .foregroundColor(.white.opacity(0.5))
                .multilineTextAlignment(.center)
                .padding(.horizontal, 32)
        }
    }

    private var scoreIndicator: some View {
        // Dot size scales with rarity — Rare/Legendary feel more impactful
        let dotSize: CGFloat = {
            switch specimen.rarity {
            case .common:    return 10
            case .uncommon:  return 12
            case .rare:      return 14
            case .legendary: return 16
            }
        }()
        let dotGlow: CGFloat = {
            switch specimen.rarity {
            case .common, .uncommon: return 0
            case .rare:              return 2
            case .legendary:         return 4
            }
        }()

        return VStack(spacing: 6) {
            // Round dots — one per total round
            HStack(spacing: 6) {
                ForEach(0..<totalRounds, id: \.self) { i in
                    Circle()
                        .fill(roundDotColor(at: i))
                        .frame(width: dotSize, height: dotSize)
                        .shadow(color: roundDotColor(at: i).opacity(0.8), radius: dotGlow)
                }
            }
            // Score text
            Text("\(roundsWon) / \(requiredWins) rounds")
                .font(.custom("JetBrainsMono-Regular", size: 11))
                .foregroundColor(.white.opacity(0.4))
        }
    }

    private func roundDotColor(at index: Int) -> Color {
        guard index < roundResults.count else {
            return index == roundsPlayed ? catColor.opacity(0.4) : Color.white.opacity(0.1) // Current round slightly highlighted
        }
        return roundResults[index] ? Color(hex: "1E8B7E") : Color(hex: "FF4D4D")
    }

    private var pitchButtonGrid: some View {
        let active = phase == .playing
        return VStack(spacing: 12) {
            HStack(spacing: 12) { pButton(0, active: active); pButton(1, active: active) }
            HStack(spacing: 12) { pButton(2, active: active); pButton(3, active: active) }
        }
    }

    private func pButton(_ idx: Int, active: Bool) -> some View {
        let lit = currentPlaybackIndex == idx
        let c = buttonColors[idx]
        return Button(action: { if active { playerTap(idx) } }) {
            RoundedRectangle(cornerRadius: 12)
                .fill(lit ? c : c.opacity(0.3))
                .frame(height: 64)
                .overlay(RoundedRectangle(cornerRadius: 12).stroke(lit ? Color.white.opacity(0.6) : c.opacity(0.4), lineWidth: lit ? 2 : 1))
                .scaleEffect(lit ? 1.05 : 1.0)
                .animation(.easeInOut(duration: 0.15), value: lit)
        }
        .disabled(!active)
    }

    @ViewBuilder
    private var actionButton: some View {
        switch phase {
        case .intro:
            Button(action: startChallenge) {
                Text("BEGIN")
                    .font(.custom("SpaceGrotesk-Bold", size: 16)).tracking(2)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity).frame(height: 50)
                    .background(RoundedRectangle(cornerRadius: 25).fill(catColor))
            }.padding(.horizontal, 40)
        case .watching:
            Text("WATCH...").font(.custom("SpaceGrotesk-Bold", size: 14)).tracking(2).foregroundColor(catColor.opacity(0.6))
        case .playing:
            Text("Round \(roundsPlayed + 1) of \(totalRounds)  ·  \(playerInput.count)/\(notesPerRound) notes")
                .font(.custom("JetBrainsMono-Regular", size: 12)).foregroundColor(.white.opacity(0.5))
        case .success, .caught:
            Text("CAUGHT!").font(.custom("SpaceGrotesk-Bold", size: 16)).tracking(2).foregroundColor(Color(hex: "1E8B7E"))
        case .escaped:
            Button(action: { dismiss() }) {
                Text("IT ESCAPED")
                    .font(.custom("SpaceGrotesk-Bold", size: 16)).tracking(2)
                    .foregroundColor(.white.opacity(0.5))
                    .frame(maxWidth: .infinity).frame(height: 50)
                    .background(RoundedRectangle(cornerRadius: 25).fill(Color.white.opacity(0.06)))
            }.padding(.horizontal, 40)
        }
    }

    // MARK: - Specimen Sound Configuration

    /// MIDI notes for the 4 catch buttons — C4, E4, G4, C5 (musical intervals)
    private static let catchMidiNotes: [Int32] = [60, 64, 67, 72]

    /// Configure the OBRIX engine to play the CAUGHT specimen's sonic character.
    /// After this call, noteOn with catchMidiNotes produces the creature's sound.
    private func configureEngineForSpecimen() {
        guard let bridge = ObrixBridge.shared() else { return }
        let cID = SpecimenCatalog.catalogSubtypeID(from: specimen.subtype)

        // Reset all modules to off
        for param in ["obrix_src1Type", "obrix_src2Type",
                      "obrix_proc1Type", "obrix_proc2Type", "obrix_proc3Type",
                      "obrix_fx1Type", "obrix_fx2Type", "obrix_fx3Type",
                      "obrix_mod1Depth", "obrix_mod2Depth"] {
            bridge.setParameterImmediate(param, value: 0)
        }
        // Ensure envelope is audible
        bridge.setParameterImmediate("obrix_ampAttack", value: 0.01)
        bridge.setParameterImmediate("obrix_ampDecay", value: 0.3)
        bridge.setParameterImmediate("obrix_ampSustain", value: 0.7)
        bridge.setParameterImmediate("obrix_ampRelease", value: 0.5)

        // Always enable a source so you hear something
        let srcTypeMap: [String: Float] = [
            "polyblep-saw": 2, "polyblep-square": 3, "polyblep-tri": 4,
            "noise-white": 5, "noise-pink": 5, "wt-analog": 6, "wt-vocal": 6, "fm-basic": 7
        ]
        bridge.setParameterImmediate("obrix_src1Type", value: srcTypeMap[cID] ?? 2)

        // If processor, also enable a filter with audible cutoff
        if specimen.category == .processor {
            let procMap: [String: Float] = [
                "svf-lp": 1, "svf-hp": 2, "svf-bp": 3,
                "shaper-soft": 4, "shaper-hard": 4, "feedback": 5
            ]
            bridge.setParameterImmediate("obrix_proc1Type", value: procMap[cID] ?? 1)
            bridge.setParameterImmediate("obrix_proc1Cutoff", value: 2000)
        }

        // If effect, enable FX
        if specimen.category == .effect {
            let fxMap: [String: Float] = [
                "delay-stereo": 1, "chorus-lush": 2, "reverb-hall": 3, "dist-warm": 1
            ]
            bridge.setParameterImmediate("obrix_fx1Type", value: fxMap[cID] ?? 1)
            bridge.setParameterImmediate("obrix_fx1Mix", value: 0.5)
        }

        // If modulator, add visible LFO modulation
        if specimen.category == .modulator {
            bridge.setParameterImmediate("obrix_mod2Type", value: 2) // LFO
            bridge.setParameterImmediate("obrix_mod2Target", value: 2) // Filter cutoff
            bridge.setParameterImmediate("obrix_mod2Depth", value: 0.4)
            bridge.setParameterImmediate("obrix_mod2Rate", value: 3.0)
            // Enable a filter so the LFO has something to sweep
            bridge.setParameterImmediate("obrix_proc1Type", value: 1) // LP filter
            bridge.setParameterImmediate("obrix_proc1Cutoff", value: 3000)
        }
    }

    // MARK: - Game Logic (Multi-Round)

    private func startChallenge() {
        currentRound = 0
        roundResults = []
        roundFeedback = .none
        configureEngineForSpecimen()
        startNextRound()
    }

    private func startNextRound() {
        // Generate fresh pattern for this round
        pattern = (0..<notesPerRound).map { _ in Int.random(in: 0...3) }
        playerInput = []
        roundFeedback = .none
        phase = .watching
        playPattern()
    }

    private func playPattern() {
        for (i, note) in pattern.enumerated() {
            let delay = Double(i) * 0.6 + 0.3
            let midiNote = Self.catchMidiNotes[note]
            DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
                currentPlaybackIndex = note
                ObrixBridge.shared()?.note(on: midiNote, velocity: 0.7)
                UIImpactFeedbackGenerator(style: .light).impactOccurred()
            }
            DispatchQueue.main.asyncAfter(deadline: .now() + delay + 0.35) {
                currentPlaybackIndex = -1
                ObrixBridge.shared()?.noteOff(midiNote)
            }
        }
        let total = Double(pattern.count) * 0.6 + 0.8
        DispatchQueue.main.asyncAfter(deadline: .now() + total) { phase = .playing }
    }

    private func playerTap(_ idx: Int) {
        guard phase == .playing, playerInput.count < pattern.count else { return }
        let step = playerInput.count
        playerInput.append(idx)

        let midiNote = Self.catchMidiNotes[idx]
        ObrixBridge.shared()?.note(on: midiNote, velocity: 0.8)
        currentPlaybackIndex = idx
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
            currentPlaybackIndex = -1; ObrixBridge.shared()?.noteOff(midiNote)
        }

        let correct = idx == pattern[step]
        if correct {
            UIImpactFeedbackGenerator(style: .light).impactOccurred()
        } else {
            UINotificationFeedbackGenerator().notificationOccurred(.warning)
            // Wrong note — round is lost immediately, skip remaining notes
            endRound(won: false)
            return
        }

        // All notes correct → round won
        if playerInput.count == pattern.count {
            endRound(won: true)
        }
    }

    private func endRound(won: Bool) {
        roundResults.append(won)
        roundFeedback = won ? .correct : .wrong

        if won {
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }

        // Check if the game is decided
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.8) {
            if alreadyWon {
                phase = .success
                UINotificationFeedbackGenerator().notificationOccurred(.success)
                catchSpecimen()
            } else if alreadyLost {
                phase = .escaped
                UINotificationFeedbackGenerator().notificationOccurred(.error)
                DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) { dismiss() }
            } else {
                // More rounds to play
                startNextRound()
            }
        }
    }

    private func catchSpecimen() {
        spectralCapture.startCapture { profile in
            let newSpecimen = SpecimenFactory.create(
                from: specimen, spectralDNA: profile,
                location: catchLocation, weather: catchWeather, accelerometer: [])
            if let _ = reefStore.addSpecimen(newSpecimen) {
                phase = .caught
                reefStore.save()
                // Advance the guided journey when a scripted specimen is caught
                if !firstLaunchManager.isJourneyComplete {
                    firstLaunchManager.advanceJourney()
                }
            } else {
                showReefFullAlert = true; return
            }
            DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) { dismiss() }
        }
    }

    // MARK: - Helpers

    private var headline: String {
        let name = SpecimenCatalog.entry(for: SpecimenCatalog.catalogSubtypeID(from: specimen.subtype))?.creatureName ?? specimen.subtype
        switch phase {
        case .intro: return "A wild \(name) appeared!"
        case .watching: return "Watch the pattern..."
        case .playing: return "Your turn!"
        case .success: return "Pattern matched!"
        case .caught: return "Specimen caught!"
        case .escaped: return "It escaped..."
        }
    }

    private var subtext: String {
        switch phase {
        case .intro: return "Match the musical pattern to catch it.\nWin \(requiredWins) out of \(totalRounds) rounds."
        case .watching: return "Round \(roundsPlayed + 1) — listen and watch."
        case .playing: return "Repeat the pattern! One wrong note ends the round."
        case .success: return "Adding to your reef..."
        case .caught: return "Added to your collection!"
        case .escaped: return "Better luck next time."
        }
    }

    private var catColor: Color {
        switch specimen.category {
        case .source: return Color(hex: "3380FF")
        case .processor: return Color(hex: "FF4D4D")
        case .modulator: return Color(hex: "4DCC4D")
        case .effect: return Color(hex: "B34DFF")
        }
    }
}

// MARK: - CoreLocation import

import CoreLocation
