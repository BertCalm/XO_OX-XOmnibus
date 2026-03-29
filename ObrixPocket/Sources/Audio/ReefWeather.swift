import Foundation

/// Weather conditions that affect the reef's sonic character
enum WeatherCondition: String, Codable, CaseIterable {
    case calm           // Default — no modification
    case morningLight   // Bright, major-leaning
    case warmAfternoon  // Full, present
    case eveningGlow    // Warm, subdued
    case nightDeep      // Dark, mysterious, minor
    case storm          // Energetic, chaotic

    var displayName: String {
        switch self {
        case .calm:          return "Calm"
        case .morningLight:  return "Morning Light"
        case .warmAfternoon: return "Warm Afternoon"
        case .eveningGlow:   return "Evening Glow"
        case .nightDeep:     return "Night Deep"
        case .storm:         return "Storm"
        }
    }

    /// Sonic character description
    var sonicDescription: String {
        switch self {
        case .calm:          return "The reef breathes gently"
        case .morningLight:  return "Bright harmonics rise with the light"
        case .warmAfternoon: return "Full voices, present and warm"
        case .eveningGlow:   return "Golden tones settle into warmth"
        case .nightDeep:     return "Deep currents carry mysterious voices"
        case .storm:         return "Wild energy crackles through the reef"
        }
    }

    /// Filter modifier (positive = brighter, negative = darker)
    var filterMod: Float {
        switch self {
        case .calm:          return 0.0
        case .morningLight:  return 0.15
        case .warmAfternoon: return 0.05
        case .eveningGlow:   return -0.05
        case .nightDeep:     return -0.15
        case .storm:         return 0.1
        }
    }

    /// LFO rate modifier (positive = faster modulation)
    var lfoRateMod: Float {
        switch self {
        case .calm:          return 0.0
        case .morningLight:  return 0.1
        case .warmAfternoon: return 0.0
        case .eveningGlow:   return -0.1
        case .nightDeep:     return -0.2
        case .storm:         return 0.4
        }
    }

    /// Reverb/space modifier
    var spaceMod: Float {
        switch self {
        case .calm:          return 0.0
        case .morningLight:  return 0.05
        case .warmAfternoon: return 0.0
        case .eveningGlow:   return 0.1
        case .nightDeep:     return 0.15
        case .storm:         return -0.1
        }
    }

    /// Tempo modifier (multiplier on base BPM)
    var tempoMultiplier: Float {
        switch self {
        case .calm:          return 1.0
        case .morningLight:  return 1.05
        case .warmAfternoon: return 1.0
        case .eveningGlow:   return 0.95
        case .nightDeep:     return 0.9
        case .storm:         return 1.15
        }
    }

    /// Which scale this weather favors (nil = no preference)
    var preferredScale: Scale? {
        switch self {
        case .calm:          return nil
        case .morningLight:  return .major
        case .warmAfternoon: return nil
        case .eveningGlow:   return .mixolydian
        case .nightDeep:     return .minor
        case .storm:         return .phrygian
        }
    }
}

/// Rare weather events that dramatically change reef behavior
enum RareWeatherEvent: String, Codable, CaseIterable {
    case bioluminescentStorm    // All specimens glow, enhanced harmonics
    case deepCurrent            // Bass voices boosted, sub frequencies
    case solarTide              // Melody voices shine, upper harmonics
    case convergence            // All affinities flip — tensions become affinities

    var displayName: String {
        switch self {
        case .bioluminescentStorm: return "Bioluminescent Storm"
        case .deepCurrent:         return "Deep Current"
        case .solarTide:           return "Solar Tide"
        case .convergence:         return "The Convergence"
        }
    }

    var description: String {
        switch self {
        case .bioluminescentStorm: return "The reef ignites with light. Every voice shimmers."
        case .deepCurrent:         return "A deep current rises. Bass voices tremble with power."
        case .solarTide:           return "Sunlight pierces the depths. Melodies crystallize."
        case .convergence:         return "The currents reverse. Everything you know changes."
        }
    }

    /// Duration in hours
    var durationHours: Int { 48 }

    /// Probability per season check (low — these are rare)
    var probability: Float {
        switch self {
        case .bioluminescentStorm: return 0.08
        case .deepCurrent:         return 0.10
        case .solarTide:           return 0.10
        case .convergence:         return 0.05
        }
    }

    /// Which roles are boosted during this event
    var boostedRoles: [MusicalRole] {
        switch self {
        case .bioluminescentStorm: return MusicalRole.allCases
        case .deepCurrent:         return [.bass]
        case .solarTide:           return [.melody, .harmony]
        case .convergence:         return []  // No boost — chemistry flip instead
        }
    }

    /// Velocity boost for boosted roles
    var velocityBoost: Float { 0.2 }

    /// Filter boost for boosted roles
    var filterBoost: Float { 0.15 }
}

/// Manages the reef's weather system on a 24-hour real-time cycle.
///
/// Weather changes automatically based on the real-world time of day.
/// Rare events occur 1-2 per season (28-day cycle) and last 48 hours.
///
/// Design: Events are NOT announced. The player discovers them by
/// noticing the reef sounds different. Curiosity, not notifications.
final class ReefWeatherManager: ObservableObject {

    // MARK: - State

    @Published var currentCondition: WeatherCondition = .calm
    @Published var activeRareEvent: RareWeatherEvent? = nil
    @Published var rareEventEndDate: Date? = nil

    /// Last time weather was checked
    private var lastWeatherCheck: Date?

    /// How often to check weather (minutes)
    private let checkInterval: TimeInterval = 15 * 60  // 15 minutes

    // MARK: - Weather Determination

    /// Update weather based on current real-world time.
    /// Call periodically (e.g., every time the app enters foreground).
    func updateWeather() {
        let hour = Calendar.current.component(.hour, from: Date())

        // Time-of-day weather
        currentCondition = conditionForHour(hour)

        // Check if rare event has expired
        if let endDate = rareEventEndDate, Date() > endDate {
            activeRareEvent = nil
            rareEventEndDate = nil
        }

        // Periodic rare event check (once per check interval)
        if let last = lastWeatherCheck {
            if Date().timeIntervalSince(last) >= checkInterval {
                checkForRareEvent()
                lastWeatherCheck = Date()
            }
        } else {
            lastWeatherCheck = Date()
        }
    }

    /// Get weather for a specific hour (0-23)
    private func conditionForHour(_ hour: Int) -> WeatherCondition {
        switch hour {
        case 5...8:   return .morningLight
        case 9...14:  return .warmAfternoon
        case 15...18: return .eveningGlow
        case 19...22: return .nightDeep
        case 23, 0...4:
            // Night: chance of storm
            return Float.random(in: 0...1) < 0.15 ? .storm : .nightDeep
        default:      return .calm
        }
    }

    /// Roll for a rare weather event
    private func checkForRareEvent() {
        // Don't roll if one is already active
        guard activeRareEvent == nil else { return }

        for event in RareWeatherEvent.allCases {
            if Float.random(in: 0...1) < event.probability {
                activeRareEvent = event
                rareEventEndDate = Date().addingTimeInterval(TimeInterval(event.durationHours * 3600))
                break  // Only one event at a time
            }
        }
    }

    // MARK: - Parameter Modifiers

    /// Get filter cutoff modifier for current weather
    var filterModifier: Float {
        var mod = currentCondition.filterMod
        if let event = activeRareEvent {
            mod += event.filterBoost
        }
        return mod
    }

    /// Get LFO rate modifier for current weather
    var lfoRateModifier: Float {
        currentCondition.lfoRateMod
    }

    /// Get space/reverb modifier for current weather
    var spaceModifier: Float {
        currentCondition.spaceMod
    }

    /// Get tempo multiplier for current weather
    var tempoMultiplier: Float {
        currentCondition.tempoMultiplier
    }

    /// Whether a specific role is boosted by the current rare event
    func isRoleBoosted(_ role: MusicalRole) -> Bool {
        guard let event = activeRareEvent else { return false }
        return event.boostedRoles.contains(role)
    }

    /// Get velocity boost for a role (0 if not boosted)
    func velocityBoost(for role: MusicalRole) -> Float {
        guard let event = activeRareEvent, event.boostedRoles.contains(role) else { return 0 }
        return event.velocityBoost
    }

    /// Whether the Convergence event is active (flips chemistry)
    var isConvergenceActive: Bool {
        activeRareEvent == .convergence
    }

    // MARK: - Persistence

    func save() {
        if let event = activeRareEvent {
            UserDefaults.standard.set(event.rawValue, forKey: "rareWeatherEvent")
            UserDefaults.standard.set(rareEventEndDate, forKey: "rareWeatherEndDate")
        } else {
            UserDefaults.standard.removeObject(forKey: "rareWeatherEvent")
            UserDefaults.standard.removeObject(forKey: "rareWeatherEndDate")
        }
    }

    func restore() {
        if let eventStr = UserDefaults.standard.string(forKey: "rareWeatherEvent"),
           let event = RareWeatherEvent(rawValue: eventStr) {
            activeRareEvent = event
            rareEventEndDate = UserDefaults.standard.object(forKey: "rareWeatherEndDate") as? Date
        }
        updateWeather()
    }
}
