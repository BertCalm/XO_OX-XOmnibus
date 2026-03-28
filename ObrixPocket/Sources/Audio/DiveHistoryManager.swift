import Foundation
import SwiftUI

struct DiveRecord: Codable, Identifiable {
    let id: UUID
    let date: Date
    let maxDepth: Int           // Meters reached
    let score: Int
    let duration: TimeInterval  // Seconds
    let specimenCount: Int      // How many specimens were in reef
    let wireCount: Int          // How many wires
    let deepestZone: String     // Zone name at max depth

    var scoreString: String { "\(score)" }
    var depthString: String { "\(maxDepth)m" }
}

final class DiveHistoryManager: ObservableObject {
    @Published var records: [DiveRecord] = []

    private let storageKey = "obrix_dive_history"
    private let maxRecords = 50

    init() { load() }

    /// Record a completed dive
    func record(_ dive: DiveRecord) {
        records.insert(dive, at: 0) // Newest first
        if records.count > maxRecords {
            records = Array(records.prefix(maxRecords))
        }
        save()
    }

    /// Best score ever
    var highScore: Int { records.map { $0.score }.max() ?? 0 }

    /// Deepest dive ever
    var deepestDive: Int { records.map { $0.maxDepth }.max() ?? 0 }

    /// Total dives
    var totalDives: Int { records.count }

    /// Average score
    var averageScore: Int {
        guard !records.isEmpty else { return 0 }
        return records.map { $0.score }.reduce(0, +) / records.count
    }

    private func save() {
        if let data = try? JSONEncoder().encode(records) {
            UserDefaults.standard.set(data, forKey: storageKey)
        }
    }

    private func load() {
        guard let data = UserDefaults.standard.data(forKey: storageKey),
              let saved = try? JSONDecoder().decode([DiveRecord].self, from: data) else { return }
        records = saved
    }
}

// MARK: - Dive History List

struct DiveHistoryList: View {
    @ObservedObject var history: DiveHistoryManager

    var body: some View {
        NavigationView {
            List(history.records) { record in
                HStack {
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Score: \(record.score)")
                            .font(.custom("JetBrainsMono-Bold", size: 14))
                            .foregroundColor(.white)
                        Text("\(record.depthString) · \(record.deepestZone)")
                            .font(.custom("Inter-Regular", size: 11))
                            .foregroundColor(.white.opacity(0.4))
                    }
                    Spacer()
                    Text(record.date, style: .date)
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.3))
                }
            }
            .listStyle(.plain)
            .navigationTitle("Dive History")
            .navigationBarTitleDisplayMode(.inline)
            .background(DesignTokens.background)
        }
    }
}
