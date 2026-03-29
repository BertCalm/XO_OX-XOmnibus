import Foundation

/// Centralizes save/restore for all game managers.
/// Replaces scattered UserDefaults access with a single coordination point.
/// Handles errors properly (logs failures instead of silent try?).
final class PersistenceCoordinator {

    static let shared = PersistenceCoordinator()

    private let encoder: JSONEncoder = {
        let e = JSONEncoder()
        e.dateEncodingStrategy = .iso8601
        // Handle NaN/Infinity in Floats (common in DSP-adjacent code)
        e.nonConformingFloatEncodingStrategy = .convertToString(
            positiveInfinity: "inf",
            negativeInfinity: "-inf",
            nan: "nan"
        )
        return e
    }()

    private let decoder: JSONDecoder = {
        let d = JSONDecoder()
        d.dateDecodingStrategy = .iso8601
        d.nonConformingFloatDecodingStrategy = .convertFromString(
            positiveInfinity: "inf",
            negativeInfinity: "-inf",
            nan: "nan"
        )
        return d
    }()

    // MARK: - Save

    func save<T: Encodable>(_ value: T, forKey key: String) {
        do {
            let data = try encoder.encode(value)
            if data.count > 500_000 {
                // Warn if a single key exceeds 500KB (UserDefaults health)
                print("[PersistenceCoordinator] WARNING: \(key) is \(data.count) bytes — consider file-based storage")
            }
            UserDefaults.standard.set(data, forKey: key)
        } catch {
            print("[PersistenceCoordinator] SAVE FAILED for \(key): \(error)")
        }
    }

    func load<T: Decodable>(_ type: T.Type, forKey key: String) -> T? {
        guard let data = UserDefaults.standard.data(forKey: key) else { return nil }
        do {
            return try decoder.decode(type, from: data)
        } catch {
            print("[PersistenceCoordinator] LOAD FAILED for \(key): \(error)")
            return nil
        }
    }

    // MARK: - File-Based Storage (for large objects)

    private var documentsURL: URL {
        FileManager.default.urls(for: .documentDirectory, in: .userDomainMask)[0]
    }

    func saveToFile<T: Encodable>(_ value: T, filename: String) {
        let url = documentsURL.appendingPathComponent(filename)
        do {
            let data = try encoder.encode(value)
            try data.write(to: url, options: .atomic)
        } catch {
            print("[PersistenceCoordinator] FILE SAVE FAILED for \(filename): \(error)")
        }
    }

    func loadFromFile<T: Decodable>(_ type: T.Type, filename: String) -> T? {
        let url = documentsURL.appendingPathComponent(filename)
        guard let data = try? Data(contentsOf: url) else { return nil }
        do {
            return try decoder.decode(type, from: data)
        } catch {
            print("[PersistenceCoordinator] FILE LOAD FAILED for \(filename): \(error)")
            return nil
        }
    }

    // MARK: - Migrate Large Stores

    /// Move a UserDefaults key to file-based storage if it exceeds the size threshold
    func migrateToFileIfNeeded(key: String, filename: String, threshold: Int = 200_000) {
        guard let data = UserDefaults.standard.data(forKey: key),
              data.count > threshold else { return }

        let url = documentsURL.appendingPathComponent(filename)
        do {
            try data.write(to: url, options: .atomic)
            UserDefaults.standard.removeObject(forKey: key)
            print("[PersistenceCoordinator] Migrated \(key) (\(data.count) bytes) to \(filename)")
        } catch {
            print("[PersistenceCoordinator] Migration failed for \(key): \(error)")
        }
    }
}
