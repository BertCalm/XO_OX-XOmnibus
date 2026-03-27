import Foundation
import CoreLocation

/// Biome types that affect spawn flavor and spectral DNA
enum Biome: String, Codable, CaseIterable {
    case coastal    // Within 5km of ocean/major water
    case forest     // Parks, nature reserves, rural
    case urban      // City centers, high density
    case elevation  // Altitude > 500m
    case nocturnal  // Anywhere, between sunset and sunrise
    case storm      // Real-time weather: rain/thunder/snow
    case liminal    // Airports, hospitals, bridges, tunnels
    case convergence // Where 2+ biomes overlap

    var displayName: String {
        switch self {
        case .coastal:     return "Coastal"
        case .forest:      return "Forest"
        case .urban:       return "Urban"
        case .elevation:   return "Elevation"
        case .nocturnal:   return "Nocturnal"
        case .storm:       return "Storm"
        case .liminal:     return "Liminal"
        case .convergence: return "Convergence"
        }
    }

    /// Spectral character bias for this biome (affects spawn flavor).
    /// 64 floats, one per FFT band — higher value = stronger presence in that frequency range.
    var spectralBias: [Float] {
        switch self {
        case .coastal:
            // Low freq emphasis — crashing waves, sub-bass rumble
            return (0..<64).map { Float($0 < 16 ? 0.8 : 0.3) }
        case .forest:
            // High freq emphasis — birdsong, wind in leaves
            return (0..<64).map { Float($0 > 32 ? 0.7 : 0.4) }
        case .urban:
            // Broadband — traffic, HVAC, crowd noise
            return Array(repeating: 0.5, count: 64)
        case .elevation:
            // Sub-bass (wind), sparse upper spectrum — thin air, distance
            return (0..<64).map { Float($0 < 8 ? 0.9 : 0.2) }
        case .nocturnal:
            // Dark — low-mid energy, quiet highs
            return (0..<64).map { Float($0 < 24 ? 0.6 : 0.15) }
        case .storm:
            // Full spectrum chaos — rain, thunder, wind
            return Array(repeating: 0.7, count: 64)
        case .liminal:
            // Eerie — sinusoidal band emphasis (fluorescent hum, PA systems)
            return (0..<64).map { Float(sin(Float($0) * 0.3) * 0.5 + 0.5) }
        case .convergence:
            // Rich — multiple overlapping biome characters
            return Array(repeating: 0.6, count: 64)
        }
    }
}

/// Detects the current biome based on location, time, and weather.
/// Uses CoreLocation for altitude and position. Coastal/forest/liminal detection
/// requires MapKit reverse geocoding (Phase 1 TODO — falls back to urban for now).
final class BiomeDetector: NSObject, ObservableObject, CLLocationManagerDelegate {
    @Published var currentBiome: Biome = .urban
    @Published var activeBiomes: Set<Biome> = [.urban]
    @Published var lastLocation: CLLocationCoordinate2D?
    @Published var locationAuthorized = false
    @Published var reefProximityEnabled = false

    /// Injected by CatchTab so WeatherService storm state can drive the .storm biome.
    var weatherService: WeatherService?

    /// Callback fired on every location update — used by CatchTab to wire SpawnManager.
    var onLocationUpdate: ((CLLocationCoordinate2D) -> Void)?

    private let locationManager = CLLocationManager()
    private var homeLocation: CLLocationCoordinate2D?

    override init() {
        super.init()
        locationManager.delegate = self
        locationManager.desiredAccuracy = kCLLocationAccuracyHundredMeters
        locationManager.distanceFilter = 100 // update every 100m
    }

    func requestAuthorization() {
        locationManager.requestWhenInUseAuthorization()
    }

    func startMonitoring() {
        // Reef Proximity bypasses live location — all spawns at home coordinate
        guard locationAuthorized, !reefProximityEnabled else { return }
        locationManager.startUpdatingLocation()
    }

    func stopMonitoring() {
        locationManager.stopUpdatingLocation()
    }

    // MARK: - Reef Proximity (accessibility mode)

    /// Pin all specimen spawns to a saved home location.
    /// Framed as reef behavior ("your reef is a homebody"), not a disability accommodation.
    func enableReefProximity(home: CLLocationCoordinate2D) {
        reefProximityEnabled = true
        homeLocation = home
        stopMonitoring()
        lastLocation = home
        detectBiomes(at: home, altitude: 0)
    }

    func disableReefProximity() {
        reefProximityEnabled = false
        homeLocation = nil
        startMonitoring()
    }

    // MARK: - CLLocationManagerDelegate

    func locationManagerDidChangeAuthorization(_ manager: CLLocationManager) {
        locationAuthorized = (manager.authorizationStatus == .authorizedWhenInUse ||
                              manager.authorizationStatus == .authorizedAlways)
        if locationAuthorized && !reefProximityEnabled {
            locationManager.startUpdatingLocation()
        }
    }

    func locationManager(_ manager: CLLocationManager, didUpdateLocations locations: [CLLocation]) {
        guard let location = locations.last else { return }
        lastLocation = location.coordinate
        detectBiomes(at: location.coordinate, altitude: location.altitude)
        onLocationUpdate?(location.coordinate)
    }

    func locationManager(_ manager: CLLocationManager, didFailWithError error: Error) {
        print("[BiomeDetector] Location update failed: \(error.localizedDescription)")
        // Keep the last known biome — don't reset to urban on transient errors
    }

    // MARK: - Storm Biome (driven by WeatherService)

    /// Toggle the .storm biome based on WeatherService real-time data.
    /// Called from CatchTab via onChange(of: weatherService.isStormActive).
    func updateStormBiome(isStorm: Bool) {
        if isStorm {
            activeBiomes.insert(.storm)
        } else {
            activeBiomes.remove(.storm)
        }
        // Rebuild convergence flag
        if activeBiomes.count >= 2 {
            activeBiomes.insert(.convergence)
        } else {
            activeBiomes.remove(.convergence)
        }
        currentBiome = resolvedBiome(from: activeBiomes)
    }

    // MARK: - Biome Detection

    private func detectBiomes(at coordinate: CLLocationCoordinate2D, altitude: CLLocationDegrees) {
        var biomes = Set<Biome>()

        // Time-based: nocturnal between 9 PM and 6 AM
        let hour = Calendar.current.component(.hour, from: Date())
        if hour < 6 || hour >= 21 {
            biomes.insert(.nocturnal)
        }

        // Altitude-based: above 500m ASL
        if altitude > 500 {
            biomes.insert(.elevation)
        }

        // TODO Phase 1: MapKit MKLocalSearch + reverse geocoding for:
        //   - .coastal  (within 5km of ocean/major lake boundary)
        //   - .forest   (parks, conservation areas, nature reserves)
        //   - .liminal  (airports by ICAO polygon, hospitals by OSM tag, bridges/tunnels)
        // TODO Phase 1: Weather API for .storm (precip type rain/snow/thunder)
        // For now, default presence biome when nothing else detected
        if biomes.isEmpty {
            biomes.insert(.urban)
        }

        // Convergence: 2+ distinct biomes active at once
        if biomes.count >= 2 {
            biomes.insert(.convergence)
        }

        activeBiomes = biomes

        // Priority: convergence > storm > nocturnal/elevation/liminal > first match > urban
        currentBiome = resolvedBiome(from: biomes)
    }

    private func resolvedBiome(from biomes: Set<Biome>) -> Biome {
        if biomes.contains(.convergence) { return .convergence }
        if biomes.contains(.storm)       { return .storm }
        if biomes.contains(.liminal)     { return .liminal }
        if biomes.contains(.elevation)   { return .elevation }
        if biomes.contains(.nocturnal)   { return .nocturnal }
        if biomes.contains(.coastal)     { return .coastal }
        if biomes.contains(.forest)      { return .forest }
        return .urban
    }
}
