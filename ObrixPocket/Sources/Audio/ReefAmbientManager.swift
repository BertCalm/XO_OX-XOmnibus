import Foundation

/// Generates a quiet ambient soundscape from the reef's specimens.
/// Plays subtle generative notes at very low velocity when the reef tab is active.
final class ReefAmbientManager: ObservableObject {
    @Published var isActive = false

    private var ambientTimer: Timer?
    private var activeAmbientNotes: Set<Int> = []
    private let maxSimultaneous = 3  // Max ambient notes at once

    /// Start the ambient soundscape
    func start(reefStore: ReefStore, audioEngine: AudioEngineManager) {
        guard !isActive else { return }
        isActive = true

        // Configure engine for the first source's chain
        if let sourceSlot = reefStore.specimens.firstIndex(where: { $0?.category == .source }) {
            audioEngine.applySlotChain(slotIndex: sourceSlot, reefStore: reefStore)
        }

        scheduleNext(reefStore: reefStore)
    }

    /// Stop all ambient sounds
    func stop() {
        isActive = false
        ambientTimer?.invalidate()
        ambientTimer = nil

        // Release all ambient notes
        for note in activeAmbientNotes {
            ObrixBridge.shared()?.noteOff(Int32(note))
        }
        activeAmbientNotes.removeAll()
    }

    private func scheduleNext(reefStore: ReefStore) {
        guard isActive else { return }

        let specimenCount = reefStore.specimens.compactMap { $0 }.count
        guard specimenCount > 0 else { return }

        // Interval: fewer specimens = more sparse, more = denser
        // 3-8 seconds between events, scaled by specimen count
        let baseInterval = 8.0 - Double(min(specimenCount, 12)) * 0.4
        let interval = baseInterval + Double.random(in: -1.0...1.0)

        ambientTimer = Timer.scheduledTimer(withTimeInterval: max(2, interval), repeats: false) { [weak self] _ in
            guard let self, self.isActive else { return }
            self.playAmbientNote(specimenCount: specimenCount)
            self.scheduleNext(reefStore: reefStore)
        }
    }

    private func playAmbientNote(specimenCount: Int) {
        // Release oldest note if at max
        if activeAmbientNotes.count >= maxSimultaneous, let oldest = activeAmbientNotes.first {
            ObrixBridge.shared()?.noteOff(Int32(oldest))
            activeAmbientNotes.remove(oldest)
        }

        // Pick a note from a pentatonic scale in a low octave
        let pentatonic = [0, 2, 4, 7, 9]
        let octave = Int.random(in: 3...4) // C3-C4 range — low and subtle
        let noteInScale = pentatonic.randomElement() ?? 0
        let midiNote = octave * 12 + noteInScale

        // Very low velocity — ambient, not performative
        let velocity = Float.random(in: 0.1...0.25)

        ObrixBridge.shared()?.note(on: Int32(midiNote), velocity: velocity)
        activeAmbientNotes.insert(midiNote)

        // Auto-release after 2-6 seconds
        let holdTime = Double.random(in: 2.0...6.0)
        DispatchQueue.main.asyncAfter(deadline: .now() + holdTime) { [weak self] in
            guard let self else { return }
            if self.activeAmbientNotes.contains(midiNote) {
                ObrixBridge.shared()?.noteOff(Int32(midiNote))
                self.activeAmbientNotes.remove(midiNote)
            }
        }
    }
}
