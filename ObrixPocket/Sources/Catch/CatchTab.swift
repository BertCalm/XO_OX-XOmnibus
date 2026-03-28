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
                catchWeather: weatherService.bestAvailable,
                onEscaped: { resetExpiryForSpecimen(id: wild.id) }
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
            // Wire reefStore for geohash persistence and load any previously visited cells
            spawnManager?.reefStore = reefStore
            spawnManager?.loadPersistedGeohashes()
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

    /// Called when a catch attempt fails — keeps the specimen on the map with a fresh 1-hour expiry.
    private func resetExpiryForSpecimen(id: UUID) {
        guard let idx = wildSpecimens.firstIndex(where: { $0.id == id }) else { return }
        let old = wildSpecimens[idx]
        let refreshed = WildSpecimen(preservingID: id, from: old, expiresAt: Date().addingTimeInterval(3600))
        wildSpecimens[idx] = refreshed
        // Mirror the reset into SpawnManager so prune logic stays consistent
        spawnManager?.updateSpecimen(refreshed, replacing: id)
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
            // Preferred-biome habitat glow — indicates this specimen is in its natural habitat
            if isPreferred {
                Circle()
                    .fill(categoryColor.opacity(0.15))
                    .frame(width: pinSize + 16, height: pinSize + 16)
            }

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

    /// True when this specimen has spawned inside one of its preferred biomes.
    private var isPreferred: Bool {
        let catalogID = SpecimenCatalog.catalogSubtypeID(from: specimen.subtype)
        if let entry = SpecimenCatalog.entry(for: catalogID) {
            return entry.preferredBiomes.contains(specimen.biome)
        }
        return false
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

// MARK: - CatchGameType

/// Which mini-game mechanic is active for this catch encounter.
/// Determined by the specimen's category — each category teaches what that module does.
enum CatchGameType {
    case patternMatch    // Sources    — Simon Says: watch and repeat the note sequence
    case frequencySweep  // Processors — drag a slider to match the target frequency band
    case rhythmTap       // Modulators — tap in time with a pulsing beat
    case echoMemory      // Effects    — same buttons as pattern match but audio is wet (harder)

    static func forCategory(_ cat: SpecimenCategory) -> CatchGameType {
        switch cat {
        case .source:    return .patternMatch
        case .processor: return .frequencySweep
        case .modulator: return .rhythmTap
        case .effect:    return .echoMemory
        }
    }
}

// MARK: - CatchScreen (Category-Specific Mini-Games)

/// Catch challenge screen with four category-specific mini-games:
///  - Sources    → Pattern Match (Simon Says)
///  - Processors → Frequency Sweep (drag slider to match cutoff)
///  - Modulators → Rhythm Tap (tap in time with the pulse)
///  - Effects    → Echo Memory (Simon Says but audio is wet)
///
/// Difficulty scales with rarity across all game types.
struct CatchScreen: View {
    let specimen: WildSpecimen
    @ObservedObject var spectralCapture: SpectralCapture
    @Binding var phase: CatchPhase
    let catchLocation: CLLocationCoordinate2D?
    let catchWeather: WeatherSnapshot
    /// Called when the player fails and chooses to keep the specimen on the map.
    var onEscaped: (() -> Void)? = nil
    @Environment(\.dismiss) private var dismiss
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager

    // MARK: Shared state
    @State private var gameType: CatchGameType = .patternMatch
    @State private var showReefFullAlert = false
    @State private var isPerfectMatch = false
    @State private var attemptNumber = 1

    // MARK: Visual polish state
    @State private var specimenBob = false
    @State private var buttonFeedback: [Int: Bool] = [:]     // per-button feedback: true = correct, false = wrong; absent = no feedback
    @State private var buttonTapScale: [Int: CGFloat] = [:]  // brief scale-up on tap
    @State private var lastRoundResult: Bool? = nil          // drives score dot bounce
    @State private var addedToStasis = false

    // Multi-round tracking (shared across all game types)
    @State private var currentRound = 0
    @State private var roundResults: [Bool] = []
    @State private var roundFeedback: RoundFeedback = .none

    enum RoundFeedback { case none, correct, wrong }

    // MARK: Pattern Match / Echo Memory state
    @State private var pattern: [Int] = []
    @State private var playerInput: [Int] = []
    @State private var currentPlaybackIndex = -1

    // MARK: Frequency Sweep state (Processors)
    @State private var sweepPosition: Float = 0.5
    @State private var sweepTarget: Float = 0.5
    @State private var sweepMatched = false
    @State private var sweepMatchTimer: Timer? = nil   // fires 0.5s after entering tolerance
    @State private var sweepTimeoutTimer: Timer? = nil // fires after 3s if never matched

    // MARK: Rhythm Tap state (Modulators)
    @State private var rhythmBPM: Double = 120
    @State private var rhythmBeatIndex = 0            // which beat is currently "on"
    @State private var rhythmHits: [Bool?] = [nil, nil, nil, nil]
    @State private var rhythmPulseScale: CGFloat = 1.0
    @State private var rhythmBeatTimes: [Date] = []   // scheduled beat timestamps
    @State private var rhythmBeatTimer: Timer? = nil
    @State private var rhythmExpectsBeats = false      // guard: only count taps when beats are live

    // MARK: Difficulty scaling
    private var notesPerRound: Int {
        switch specimen.rarity {
        case .common: return 3; case .uncommon: return 4
        case .rare: return 4; case .legendary: return 5
        }
    }

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

    private var roundsWon: Int  { roundResults.filter { $0 }.count }
    private var roundsLost: Int { roundResults.filter { !$0 }.count }
    private var roundsPlayed: Int { roundResults.count }

    private var canStillWin: Bool { roundsWon + (totalRounds - roundsPlayed) >= requiredWins }
    private var alreadyWon: Bool  { roundsWon >= requiredWins }
    private var alreadyLost: Bool { !canStillWin }

    private var buttonColors: [Color] {
        [catColor.opacity(0.5), catColor.opacity(0.7), catColor.opacity(0.85), catColor]
    }

    // MARK: - Biome + Time Ambience

    private var biomeGradient: LinearGradient {
        let colors: [Color]
        switch specimen.biome {
        case .coastal:     colors = [Color(hex: "0A0A0F"), Color(hex: "0A1520")] // Deep blue tint
        case .forest:      colors = [Color(hex: "0A0A0F"), Color(hex: "0A150A")] // Deep green tint
        case .urban:       colors = [Color(hex: "0A0A0F"), Color(hex: "12100A")] // Warm amber tint
        case .elevation:   colors = [Color(hex: "0A0A0F"), Color(hex: "0F0A15")] // Cool purple tint
        case .nocturnal:   colors = [Color(hex: "050508"), Color(hex: "0A0A0F")] // Extra dark
        case .storm:       colors = [Color(hex: "0A0A0F"), Color(hex: "15100A")] // Storm amber
        case .liminal:     colors = [Color(hex: "0A0A0F"), Color(hex: "0F0F12")] // Steel grey tint
        case .convergence: colors = [Color(hex: "0A0A0F"), Color(hex: "14120A")] // Gold tint
        }
        return LinearGradient(colors: colors, startPoint: .top, endPoint: .bottom)
    }

    private var timeIndicator: some View {
        let hour = Calendar.current.component(.hour, from: Date())
        let isNight = hour < 6 || hour >= 21
        return HStack(spacing: 4) {
            Image(systemName: isNight ? "moon.stars.fill" : "sun.max.fill")
                .font(.system(size: 8))
                .foregroundColor(isNight ? Color(hex: "A8D8EA").opacity(0.4) : Color(hex: "E9C46A").opacity(0.4))
            Text(specimen.biome.displayName)
                .font(.custom("JetBrainsMono-Regular", size: 8))
                .foregroundColor(.white.opacity(0.2))
        }
    }

    // MARK: Body

    var body: some View {
        ZStack {
            biomeGradient.ignoresSafeArea()

            VStack(spacing: 20) {
                Spacer()
                specimenHeader
                statusText
                if phase != .intro { scoreIndicator }
                gameView.padding(.horizontal, 40)
                actionButton
                Spacer()
            }

            // Time-of-day indicator — top-right corner
            VStack {
                HStack {
                    Spacer()
                    timeIndicator
                        .padding(.top, 16)
                        .padding(.trailing, 20)
                }
                Spacer()
            }
        }
        .onAppear {
            phase = .intro
            gameType = CatchGameType.forCategory(specimen.category)
        }
    }

    // MARK: - Game view dispatch

    @ViewBuilder
    private var gameView: some View {
        switch gameType {
        case .patternMatch, .echoMemory:
            pitchButtonGrid
        case .frequencySweep:
            frequencySweepView
        case .rhythmTap:
            rhythmTapView
        }
    }

    private var specimenHeader: some View {
        VStack(spacing: 8) {
            ZStack {
                // CAUGHT: green glow halo behind the specimen
                if phase == .caught || phase == .success {
                    Circle()
                        .fill(Color(hex: "1E8B7E").opacity(0.15))
                        .frame(width: 180, height: 180)
                        .scaleEffect(1.2)
                        .animation(.easeInOut(duration: 0.5), value: phase == .caught || phase == .success)
                }

                Circle()
                    .fill(catColor.opacity(0.12))
                    .frame(width: 140, height: 140)
                let cID = SpecimenCatalog.catalogSubtypeID(from: specimen.subtype)
                SpecimenSprite(subtype: cID, category: specimen.category, size: 70)
                Circle()
                    .stroke(catColor.opacity(0.5), lineWidth: specimen.rarity == .legendary ? 4 : (specimen.rarity == .rare ? 3 : (specimen.rarity == .uncommon ? 2 : 1)))
                    .frame(width: 130, height: 130)
            }
            // Breathing bob — active during game phases, disabled on end states
            .offset(y: specimenBob ? -3 : 3)
            .animation(
                (phase == .caught || phase == .escaped)
                    ? .default
                    : .easeInOut(duration: 1.5).repeatForever(autoreverses: true),
                value: specimenBob
            )
            // ESCAPED: fade + shrink
            .opacity(phase == .escaped ? 0.3 : 1.0)
            .scaleEffect(phase == .escaped ? 0.7 : 1.0)
            .animation(.easeInOut(duration: 0.5), value: phase == .escaped)
            .onAppear { specimenBob = true }
            let cID = SpecimenCatalog.catalogSubtypeID(from: specimen.subtype)
            Text(SpecimenCatalog.entry(for: cID)?.creatureName ?? specimen.subtype)
                .font(.custom("SpaceGrotesk-Bold", size: 18))
                .foregroundColor(.white)
            Text(specimen.rarity.rawValue.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(Color(hex: "E9C46A"))
            // Ambient context — biome indicator with storm icon when relevant
            HStack(spacing: 4) {
                if specimen.biome == .storm {
                    Image(systemName: "cloud.bolt.fill")
                        .font(.system(size: 8))
                        .foregroundColor(.white.opacity(0.25))
                }
                Text(specimen.biome.displayName)
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.25))
            }
            if phase == .intro {
                if attemptNumber > 1 {
                    Text("Attempt \(attemptNumber)")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.3))
                }
                if let entry = SpecimenCatalog.entry(for: cID) {
                    Text(entry.personalityLine)
                        .font(.custom("Inter-Regular", size: 11))
                        .foregroundColor(.white.opacity(0.4))
                        .italic()
                        .multilineTextAlignment(.center)
                        .padding(.horizontal, 32)
                }
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
                        // Bounce the dot that was just resolved
                        .scaleEffect(i == roundsPlayed - 1 && lastRoundResult != nil ? 1.4 : 1.0)
                        .animation(.spring(response: 0.3, dampingFraction: 0.45), value: lastRoundResult)
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
        let tapScale = buttonTapScale[idx] ?? 1.0
        let c = buttonColors[idx]
        let feedbackResult: Bool? = buttonFeedback[idx]  // nil = absent (no feedback shown)

        return Button(action: { if active { playerTap(idx) } }) {
            RoundedRectangle(cornerRadius: 12)
                .fill(lit ? c : c.opacity(0.3))
                .frame(height: 64)
                .overlay(RoundedRectangle(cornerRadius: 12).stroke(lit ? Color.white.opacity(0.6) : c.opacity(0.4), lineWidth: lit ? 2 : 1))
                // Correct/wrong feedback icon overlay
                .overlay(
                    Group {
                        if let result = feedbackResult {
                            Image(systemName: result ? "checkmark" : "xmark")
                                .font(.system(size: 16, weight: .bold))
                                .foregroundColor(result ? Color(hex: "1E8B7E") : Color(hex: "FF4D4D"))
                        }
                    }
                )
                // Playback pulse + tap scale animations
                .scaleEffect(lit ? 1.08 : tapScale)
                .animation(lit ? .spring(response: 0.15, dampingFraction: 0.4) : .easeInOut(duration: 0.15), value: lit)
                .animation(.spring(response: 0.18, dampingFraction: 0.4), value: tapScale)
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
            playingStatusLine
        case .success, .caught:
            Text("CAUGHT!").font(.custom("SpaceGrotesk-Bold", size: 16)).tracking(2).foregroundColor(Color(hex: "1E8B7E"))
        case .escaped:
            VStack(spacing: 8) {
                Button(action: {
                    attemptNumber += 1
                    // Reset round state and restart the challenge
                    currentRound = 0
                    roundResults = []
                    roundFeedback = .none
                    isPerfectMatch = false
                    pattern = []
                    playerInput = []
                    currentPlaybackIndex = -1
                    // Cancel any in-flight sweep timers
                    sweepMatched = false
                    sweepMatchTimer?.invalidate(); sweepMatchTimer = nil
                    sweepTimeoutTimer?.invalidate(); sweepTimeoutTimer = nil
                    // Stop rhythm sequence
                    rhythmExpectsBeats = false
                    rhythmHits = [nil, nil, nil, nil]
                    phase = .intro
                }) {
                    Text("TRY AGAIN")
                        .font(.custom("SpaceGrotesk-Bold", size: 16)).tracking(2)
                        .foregroundColor(.white)
                        .frame(maxWidth: .infinity).frame(height: 50)
                        .background(RoundedRectangle(cornerRadius: 25).fill(catColor.opacity(0.5)))
                }
                .padding(.horizontal, 40)

                Button(action: {
                    onEscaped?()
                    dismiss()
                }) {
                    Text("Let It Go")
                        .font(.custom("Inter-Regular", size: 13))
                        .foregroundColor(.white.opacity(0.3))
                }
            }
        }
    }

    // MARK: - Playing Status Line

    /// Status line shown in .playing phase — content varies by game type.
    @ViewBuilder
    private var playingStatusLine: some View {
        switch gameType {
        case .patternMatch, .echoMemory:
            Text("Round \(roundsPlayed + 1) of \(totalRounds)  ·  \(playerInput.count)/\(notesPerRound) notes")
                .font(.custom("JetBrainsMono-Regular", size: 12)).foregroundColor(.white.opacity(0.5))
        case .frequencySweep:
            Text("Round \(roundsPlayed + 1) of \(totalRounds)  ·  drag to match")
                .font(.custom("JetBrainsMono-Regular", size: 12)).foregroundColor(.white.opacity(0.5))
        case .rhythmTap:
            let hits = rhythmHits.compactMap { $0 }.filter { $0 }.count
            let scored = rhythmHits.compactMap { $0 }.count
            Text("Round \(roundsPlayed + 1) of \(totalRounds)  ·  \(hits)/\(scored) on beat")
                .font(.custom("JetBrainsMono-Regular", size: 12)).foregroundColor(.white.opacity(0.5))
        }
    }

    // MARK: - Frequency Sweep View (Processors)

    /// Horizontal slider mini-game: drag to match a target frequency band.
    /// The engine's filter cutoff updates in real time so the player hears the match.
    private var frequencySweepView: some View {
        VStack(spacing: 16) {
            Text("FREQUENCY SWEEP")
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .tracking(1.5)
                .foregroundColor(catColor.opacity(0.6))

            // Track + target zone + thumb — laid out with GeometryReader
            GeometryReader { geo in
                ZStack(alignment: .leading) {
                    // Background track
                    RoundedRectangle(cornerRadius: 4)
                        .fill(Color.white.opacity(0.08))
                        .frame(height: 8)

                    // Target zone (±0.08 tolerance, shown as glowing band)
                    let zoneWidth = geo.size.width * 0.16
                    let targetX = CGFloat(sweepTarget) * geo.size.width - zoneWidth / 2
                    RoundedRectangle(cornerRadius: 4)
                        .fill(catColor.opacity(sweepMatched ? 0.6 : 0.3))
                        .frame(width: zoneWidth, height: 8)
                        .offset(x: max(0, min(geo.size.width - zoneWidth, targetX)))
                        .animation(.easeInOut(duration: 0.2), value: sweepMatched)

                    // Player thumb
                    let thumbX = CGFloat(sweepPosition) * (geo.size.width - 20)
                    Circle()
                        .fill(sweepMatched ? Color(hex: "1E8B7E") : catColor)
                        .frame(width: 20, height: 20)
                        .offset(x: max(0, min(geo.size.width - 20, thumbX)))
                        .animation(.interactiveSpring(), value: sweepPosition)
                }
            }
            .frame(height: 24)

            // Invisible slider for interaction (on top of the visual track)
            Slider(value: Binding(
                get: { Double(sweepPosition) },
                set: { newVal in
                    guard phase == .playing else { return }
                    sweepPosition = Float(newVal)
                    // Scrub the engine filter cutoff in real time: 200–15200 Hz
                    ObrixBridge.shared()?.setParameterImmediate("obrix_proc1Cutoff",
                                                                value: 200 + sweepPosition * 15000)
                    checkSweepMatch()
                }
            ), in: 0...1)
            .tint(catColor)
            .disabled(phase != .playing)

            // Frequency spectrum visualisation — deterministic shape derived from sweep position
            HStack(spacing: 2) {
                ForEach(0..<20, id: \.self) { i in
                    let normalizedI = Float(i) / 19.0
                    let dist = abs(normalizedI - sweepPosition)
                    let barHeight = CGFloat(max(4, 20 - dist * 50))
                    let inZone = dist < 0.1
                    RoundedRectangle(cornerRadius: 1)
                        .fill(catColor.opacity(inZone ? (sweepMatched ? 0.8 : 0.55) : 0.22))
                        .frame(width: 4, height: barHeight)
                        .animation(.easeInOut(duration: 0.1), value: sweepPosition)
                }
            }
            .frame(height: 24)
            .padding(.horizontal, 40)

            Text(sweepMatched ? "MATCHED!" : "Find the frequency...")
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(sweepMatched ? Color(hex: "1E8B7E") : .white.opacity(0.4))
                .animation(.easeInOut(duration: 0.15), value: sweepMatched)
        }
    }

    // MARK: - Rhythm Tap View (Modulators)

    /// Pulsing-circle tap target with four beat indicator dots.
    private var rhythmTapView: some View {
        VStack(spacing: 16) {
            Text("RHYTHM TAP")
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .tracking(1.5)
                .foregroundColor(catColor.opacity(0.6))

            // Four beat indicator dots
            HStack(spacing: 14) {
                ForEach(0..<4, id: \.self) { i in
                    Circle()
                        .fill(rhythmDotColor(i))
                        .frame(width: 14, height: 14)
                        .scaleEffect(rhythmBeatIndex == i && phase == .playing ? 1.4 : 1.0)
                        .animation(.easeOut(duration: 0.1), value: rhythmBeatIndex)
                }
            }

            // Big tap target — spring pulse on each beat
            Button(action: rhythmPlayerTap) {
                ZStack {
                    Circle()
                        .fill(catColor.opacity(0.18))
                        .frame(width: 110, height: 110)
                        .scaleEffect(rhythmPulseScale)
                        .animation(.spring(response: 0.15, dampingFraction: 0.3), value: rhythmPulseScale)

                    Circle()
                        .stroke(catColor, lineWidth: 2)
                        .frame(width: 110, height: 110)
                        .scaleEffect(rhythmPulseScale)
                        .animation(.spring(response: 0.15, dampingFraction: 0.3), value: rhythmPulseScale)

                    Text("TAP")
                        .font(.custom("SpaceGrotesk-Bold", size: 16))
                        .foregroundColor(catColor)
                }
            }
            .disabled(phase != .playing)

            Text("\(Int(rhythmBPM)) BPM")
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(.white.opacity(0.3))
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

    // MARK: - Game Logic (Multi-Round Dispatcher)

    private func startChallenge() {
        currentRound = 0
        roundResults = []
        roundFeedback = .none
        isPerfectMatch = false
        configureEngineForSpecimen()
        startNextRound()
    }

    private func startNextRound() {
        roundFeedback = .none
        switch gameType {
        case .patternMatch, .echoMemory:
            startPatternRound()
        case .frequencySweep:
            startSweepRound()
        case .rhythmTap:
            startRhythmRound()
        }
    }

    // MARK: Pattern Match / Echo Memory round

    private func startPatternRound() {
        pattern = (0..<notesPerRound).map { _ in Int.random(in: 0...3) }
        playerInput = []
        phase = .watching
        if gameType == .echoMemory {
            // Heavy wet effect — makes it harder to track individual notes
            ObrixBridge.shared()?.setParameterImmediate("obrix_fx1Mix", value: 0.7)
        }
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

        // Brief scale-up on tap
        buttonTapScale[idx] = 1.12
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.12) {
            buttonTapScale[idx] = 1.0
        }

        DispatchQueue.main.asyncAfter(deadline: .now() + 0.2) {
            currentPlaybackIndex = -1; ObrixBridge.shared()?.noteOff(midiNote)
        }

        let correct = idx == pattern[step]
        // Show checkmark/X feedback on the tapped button
        buttonFeedback[idx] = correct
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.35) {
            buttonFeedback.removeValue(forKey: idx)
        }

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

    // MARK: Frequency Sweep round (Processors)

    private func startSweepRound() {
        let target = Float.random(in: 0.1...0.9)
        sweepTarget = target
        sweepPosition = 0.5 // reset thumb to centre each round
        sweepMatched = false
        sweepMatchTimer?.invalidate()
        sweepMatchTimer = nil
        sweepTimeoutTimer?.invalidate()
        sweepTimeoutTimer = nil

        // Play a sustained reference tone at the target cutoff so the player hears what to find
        ObrixBridge.shared()?.setParameterImmediate("obrix_proc1Cutoff",
                                                    value: 200 + target * 15000)
        ObrixBridge.shared()?.note(on: 60, velocity: 0.6)
        DispatchQueue.main.asyncAfter(deadline: .now() + 1.2) {
            ObrixBridge.shared()?.noteOff(60)
        }

        phase = .playing

        // 3-second timeout if the player never lands in the zone
        sweepTimeoutTimer = Timer.scheduledTimer(withTimeInterval: 3.0, repeats: false) { [self] _ in
            guard self.phase == .playing else { return }
            self.sweepMatchTimer?.invalidate()
            self.sweepMatchTimer = nil
            UINotificationFeedbackGenerator().notificationOccurred(.warning)
            self.endRound(won: false)
        }
    }

    /// Called every time the sweep slider value changes. Detects entry/exit of the tolerance zone.
    private func checkSweepMatch() {
        let inZone = abs(sweepPosition - sweepTarget) <= 0.08
        if inZone && !sweepMatched {
            sweepMatched = true
            UIImpactFeedbackGenerator(style: .medium).impactOccurred()
            // Must hold inside the zone for 0.5 s to confirm
            sweepMatchTimer = Timer.scheduledTimer(withTimeInterval: 0.5, repeats: false) { [self] _ in
                guard self.sweepMatched, abs(self.sweepPosition - self.sweepTarget) <= 0.08 else {
                    self.sweepMatched = false
                    return
                }
                self.sweepTimeoutTimer?.invalidate()
                self.sweepTimeoutTimer = nil
                UINotificationFeedbackGenerator().notificationOccurred(.success)
                self.endRound(won: true)
            }
        } else if !inZone && sweepMatched {
            sweepMatched = false
            sweepMatchTimer?.invalidate()
            sweepMatchTimer = nil
        }
    }

    // MARK: Rhythm Tap round (Modulators)

    private var rhythmBPMForRarity: Double {
        switch specimen.rarity {
        case .common:    return 90
        case .uncommon:  return 110
        case .rare:      return 130
        case .legendary: return 150
        }
    }

    private func startRhythmRound() {
        rhythmBPM = rhythmBPMForRarity
        rhythmHits = [nil, nil, nil, nil]
        rhythmBeatIndex = -1
        rhythmExpectsBeats = false
        rhythmBeatTimer?.invalidate()

        let interval = 60.0 / rhythmBPM
        let startTime = Date().addingTimeInterval(0.6)
        rhythmBeatTimes = (0..<4).map { i in startTime.addingTimeInterval(Double(i) * interval) }

        phase = .playing
        rhythmExpectsBeats = true

        for (i, beatTime) in rhythmBeatTimes.enumerated() {
            let delay = beatTime.timeIntervalSinceNow
            DispatchQueue.main.asyncAfter(deadline: .now() + delay) {
                guard self.rhythmExpectsBeats else { return }
                self.rhythmBeatIndex = i
                self.rhythmPulseScale = 1.25
                UIImpactFeedbackGenerator(style: .rigid).impactOccurred()
                ObrixBridge.shared()?.note(on: 36, velocity: 0.5)
                DispatchQueue.main.asyncAfter(deadline: .now() + 0.08) {
                    ObrixBridge.shared()?.noteOff(36)
                    self.rhythmPulseScale = 1.0
                }
            }
        }

        let evalDelay = rhythmBeatTimes.last!.timeIntervalSinceNow + interval
        DispatchQueue.main.asyncAfter(deadline: .now() + evalDelay) {
            guard self.rhythmExpectsBeats else { return }
            self.rhythmExpectsBeats = false
            self.evaluateRhythmRound()
        }
    }

    private func rhythmPlayerTap() {
        guard phase == .playing, rhythmExpectsBeats else { return }
        let now = Date()
        UIImpactFeedbackGenerator(style: .light).impactOccurred()
        ObrixBridge.shared()?.note(on: 60, velocity: 0.75)
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.15) { ObrixBridge.shared()?.noteOff(60) }

        let tolerance: TimeInterval = 0.080
        for (i, beatTime) in rhythmBeatTimes.enumerated() {
            if rhythmHits[i] == nil, abs(now.timeIntervalSince(beatTime)) <= tolerance {
                rhythmHits[i] = true
                return
            }
        }
        for i in 0..<4 where rhythmHits[i] == nil {
            rhythmHits[i] = false
            UINotificationFeedbackGenerator().notificationOccurred(.warning)
            return
        }
    }

    private func evaluateRhythmRound() {
        for i in 0..<4 where rhythmHits[i] == nil { rhythmHits[i] = false }
        let hits = rhythmHits.compactMap { $0 }.filter { $0 }.count
        endRound(won: hits >= 3)
    }

    private func rhythmDotColor(_ i: Int) -> Color {
        switch rhythmHits[i] {
        case .some(true):  return Color(hex: "1E8B7E")
        case .some(false): return Color(hex: "FF4D4D")
        case .none:        return i == rhythmBeatIndex ? catColor : catColor.opacity(0.2)
        }
    }

    // MARK: Shared round outcome

    private func endRound(won: Bool) {
        // Cancel any in-flight sweep timers before mutating state
        if gameType == .frequencySweep {
            sweepMatchTimer?.invalidate(); sweepMatchTimer = nil
            sweepTimeoutTimer?.invalidate(); sweepTimeoutTimer = nil
        }
        if gameType == .rhythmTap {
            rhythmExpectsBeats = false
        }

        roundResults.append(won)
        roundFeedback = won ? .correct : .wrong

        // Drive score dot bounce
        lastRoundResult = won
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { lastRoundResult = nil }

        if won {
            UINotificationFeedbackGenerator().notificationOccurred(.success)
        }

        // Check if the game is decided
        DispatchQueue.main.asyncAfter(deadline: .now() + 0.8) {
            if self.alreadyWon {
                let perfectMatch = (self.roundsWon == self.totalRounds)
                self.isPerfectMatch = perfectMatch
                self.phase = .success
                if perfectMatch {
                    UINotificationFeedbackGenerator().notificationOccurred(.success)
                    UIImpactFeedbackGenerator(style: .heavy).impactOccurred()
                } else {
                    UINotificationFeedbackGenerator().notificationOccurred(.success)
                }
                self.catchSpecimen(perfectMatch: perfectMatch)
            } else if self.alreadyLost {
                self.phase = .escaped
                UINotificationFeedbackGenerator().notificationOccurred(.error)
                // No auto-dismiss — let the player choose Retry or Let It Go
            } else {
                self.startNextRound()
            }
        }
    }

    private func catchSpecimen(perfectMatch: Bool = false) {
        spectralCapture.startCapture { profile in
            var newSpecimen = SpecimenFactory.create(
                from: specimen, spectralDNA: profile,
                location: catchLocation, weather: catchWeather, accelerometer: [])

            // Bonus Round: perfect match upgrades rarity by one tier
            if perfectMatch && newSpecimen.rarity != .legendary {
                let upgraded: SpecimenRarity
                switch newSpecimen.rarity {
                case .common:    upgraded = .uncommon
                case .uncommon:  upgraded = .rare
                case .rare:      upgraded = .legendary
                case .legendary: upgraded = .legendary
                }
                newSpecimen.rarity = upgraded
                // Update display name to reflect the new rarity
                let catalogID = SpecimenCatalog.catalogSubtypeID(from: newSpecimen.subtype)
                if let entry = SpecimenCatalog.entry(for: catalogID) {
                    newSpecimen.name = entry.displayName(rarity: upgraded, morphIndex: newSpecimen.morphIndex)
                }
            }

            if perfectMatch {
                ReefStatsTracker.shared.increment(.perfectCatches)
            }

            if let _ = reefStore.addSpecimen(newSpecimen) {
                // Reef has a free slot — place it directly
                addedToStasis = false
                phase = .caught
                reefStore.save()
                ReefStatsTracker.shared.increment(.specimensCaught)
                // Advance the guided journey when a scripted specimen is caught
                if !firstLaunchManager.isJourneyComplete {
                    firstLaunchManager.advanceJourney()
                }
            } else {
                // Reef is full — save to stasis so the specimen is never lost
                reefStore.saveSpecimenToStasis(newSpecimen)
                addedToStasis = true
                phase = .caught
                ReefStatsTracker.shared.increment(.specimensCaught)
                // Journey advancement still applies even when sent to stasis
                if !firstLaunchManager.isJourneyComplete {
                    firstLaunchManager.advanceJourney()
                }
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
        case .playing:
            switch gameType {
            case .patternMatch, .echoMemory: return "Your turn!"
            case .frequencySweep:            return "Find the frequency!"
            case .rhythmTap:                 return "Tap with the beat!"
            }
        case .success: return isPerfectMatch ? "PERFECT MATCH!" : "Well done!"
        case .caught:
            if addedToStasis { return "Sent to Stasis" }
            return isPerfectMatch ? "Rarity upgraded!" : "Specimen caught!"
        case .escaped: return "It got away!"
        }
    }

    private var subtext: String {
        switch phase {
        case .intro:
            switch gameType {
            case .patternMatch:
                return "Match the musical pattern.\nWin \(requiredWins) of \(totalRounds) rounds."
            case .frequencySweep:
                return "Sweep to find the frequency.\nMatch \(requiredWins) of \(totalRounds) targets."
            case .rhythmTap:
                return "Tap in time with the pulse.\nNail \(requiredWins) of \(totalRounds) rhythms."
            case .echoMemory:
                return "Remember through the echo.\nWin \(requiredWins) of \(totalRounds) rounds."
            }
        case .watching:
            return "Round \(roundsPlayed + 1) — listen and watch."
        case .playing:
            switch gameType {
            case .patternMatch:   return "Repeat the pattern! One wrong note ends the round."
            case .echoMemory:     return "The echo obscures it. Remember the dry sequence."
            case .frequencySweep: return "Drag the slider. Hold inside the glow for 0.5 s."
            case .rhythmTap:      return "Tap when each dot lights up. 3 of 4 on beat wins."
            }
        case .success:
            if isPerfectMatch {
                let upgraded = upgradedRarity(from: specimen.rarity)
                return "Rarity upgraded! → \(upgraded.rawValue.capitalized)"
            }
            return "Adding to your reef..."
        case .caught:
            if addedToStasis { return "Reef full — preserved in stasis. Visit the reef to place it." }
            return isPerfectMatch ? "Added to your reef with a bonus!" : "Added to your collection!"
        case .escaped: return "Try again later."
        }
    }

    private func upgradedRarity(from rarity: SpecimenRarity) -> SpecimenRarity {
        switch rarity {
        case .common:    return .uncommon
        case .uncommon:  return .rare
        case .rare:      return .legendary
        case .legendary: return .legendary
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
