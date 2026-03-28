import Foundation
import UIKit

/// Simple metronome with visual + haptic beat indication.
/// BPM syncs to the arpeggiator when arp mode is active.
final class MetronomeManager: ObservableObject {
    @Published var isRunning = false
    @Published var bpm: Double = 120 {
        didSet { if isRunning { restart() } }
    }
    @Published var currentBeat: Int = 0  // 0-3 (4/4 time)
    @Published var beatsPerBar: Int = 4

    private var timer: Timer?

    var beatInterval: TimeInterval { 60.0 / bpm }

    func start() {
        guard !isRunning else { return }
        isRunning = true
        currentBeat = 0
        scheduleBeat()
    }

    func stop() {
        timer?.invalidate()
        timer = nil
        isRunning = false
        currentBeat = 0
    }

    func toggle() {
        if isRunning { stop() } else { start() }
    }

    private func restart() {
        if isRunning {
            timer?.invalidate()
            scheduleBeat()
        }
    }

    private func scheduleBeat() {
        timer = Timer.scheduledTimer(withTimeInterval: beatInterval, repeats: true) { [weak self] _ in
            guard let self, self.isRunning else { return }
            self.currentBeat = (self.currentBeat + 1) % self.beatsPerBar

            // Haptic: downbeat = heavy, other beats = light
            if self.currentBeat == 0 {
                UIImpactFeedbackGenerator(style: .heavy).impactOccurred()
            } else {
                UIImpactFeedbackGenerator(style: .light).impactOccurred()
            }
        }
    }
}
