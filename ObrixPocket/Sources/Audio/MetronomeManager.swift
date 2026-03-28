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
    private let heavyHaptic = UIImpactFeedbackGenerator(style: .heavy)
    private let lightHaptic = UIImpactFeedbackGenerator(style: .light)

    var beatInterval: TimeInterval { 60.0 / bpm }

    func start() {
        guard !isRunning else { return }
        isRunning = true
        currentBeat = 0
        heavyHaptic.prepare()
        lightHaptic.prepare()
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
        let t = Timer(timeInterval: beatInterval, repeats: true) { [weak self] _ in
            guard let self, self.isRunning else { return }
            self.currentBeat = (self.currentBeat + 1) % self.beatsPerBar

            // Haptic: downbeat = heavy, other beats = light
            if self.currentBeat == 0 {
                self.heavyHaptic.impactOccurred()
                self.heavyHaptic.prepare() // re-prepare for next downbeat
            } else {
                self.lightHaptic.impactOccurred()
                self.lightHaptic.prepare() // re-prepare for next beat
            }
        }
        RunLoop.main.add(t, forMode: .common)
        timer = t
    }
}
