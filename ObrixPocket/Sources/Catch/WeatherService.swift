import Foundation
import CoreLocation
import WeatherKit

/// Fetches weather data via Apple WeatherKit for spectral DNA seeding and biome detection.
/// Spec Section 3.4: WeatherKit integration, 1-hour cache, 6-hour offline fallback.
@MainActor
final class WeatherService: ObservableObject {
    @Published var currentWeather: WeatherSnapshot?
    @Published var isStormActive = false

    private let appleWeather = AppleWeatherAdapter()
    private var lastFetchTime: Date?
    private let cacheDuration: TimeInterval = 3600   // 1-hour cache
    private let offlineFallbackDuration: TimeInterval = 21600 // 6-hour window for cached data

    // MARK: - Fetch

    /// Fetch weather for the given location, respecting the 1-hour cache.
    /// Safe to call on every location update — returns early if cache is fresh.
    func fetchWeather(at location: CLLocationCoordinate2D) async {
        // Return early if cache is still fresh
        if let lastTime = lastFetchTime,
           Date().timeIntervalSince(lastTime) < cacheDuration,
           currentWeather != nil {
            return
        }

        do {
            let weather = try await appleWeather.weather(
                for: CLLocation(latitude: location.latitude, longitude: location.longitude)
            )

            let current = weather.currentWeather
            let snapshot = WeatherSnapshot(
                temperature: Float(current.temperature.converted(to: .celsius).value),
                humidity: Float(current.humidity),
                pressure: Float(current.pressure.converted(to: .hectopascals).value),
                description: current.condition.description
            )

            self.currentWeather = snapshot
            self.lastFetchTime = Date()

            // Storm detection: drives .storm biome in BiomeDetector
            self.isStormActive = Self.isStormCondition(current.condition)
        } catch {
            // Offline / quota fallback — keep cached snapshot if still within 6-hour window
            if currentWeather == nil {
                self.currentWeather = .fairWeather
            }
            print("[WeatherService] Fetch failed (using cache/fallback): \(error)")
        }
    }

    // MARK: - Best Available Snapshot

    /// Returns the most relevant weather snapshot available.
    /// Uses cached data if within the 6-hour offline window; otherwise returns the
    /// "Weather Unknown" fair-weather default (tag applied by SpecimenFactory).
    var bestAvailable: WeatherSnapshot {
        if let cached = currentWeather,
           let lastTime = lastFetchTime,
           Date().timeIntervalSince(lastTime) < offlineFallbackDuration {
            return cached
        }
        return .fairWeather
    }

    // MARK: - Storm Detection

    /// Returns true for precipitation/severe conditions that should activate the .storm biome.
    private static func isStormCondition(_ condition: WeatherKit.WeatherCondition) -> Bool {
        switch condition {
        case .rain, .heavyRain, .drizzle,
             .thunderstorms, .isolatedThunderstorms, .scatteredThunderstorms, .strongStorms,
             .snow, .heavySnow, .blizzard, .blowingSnow, .flurries, .freezingRain, .sleet,
             .tropicalStorm, .hurricane, .hail, .wintryMix:
            return true
        default:
            return false
        }
    }
}

// MARK: - WeatherKit Adapter

/// Wraps WeatherKit.WeatherService.shared to avoid a name collision with our WeatherService class.
private final class AppleWeatherAdapter {
    private let service = WeatherKit.WeatherService.shared

    func weather(for location: CLLocation) async throws -> Weather {
        try await service.weather(for: location)
    }
}
