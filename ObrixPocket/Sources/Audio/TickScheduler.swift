import Foundation

/// Centralized timer multiplexer — one Timer drives all high-frequency callbacks.
///
/// Instead of N independent Timer objects (each a kernel timer), TickScheduler
/// runs a single 60 Hz Timer on `.common` RunLoop mode and dispatches to
/// registered callbacks at their requested cadence via frame-skipping.
///
/// `.common` mode is critical: default-mode timers pause during scroll gestures
/// (`UITrackingRunLoopMode`), causing audible jitter in metronomes, recorders,
/// and dive timers. One timer, one mode, one lifecycle.
///
/// Usage:
/// ```
/// // Register a 30 Hz callback
/// token = TickScheduler.shared.register(hz: 30) { [weak self] in
///     self?.updatePlaybackPosition()
/// }
///
/// // Cancel by nilling the token (or let it deinit)
/// token = nil
/// ```
final class TickScheduler {
    static let shared = TickScheduler()

    private var timer: Timer?
    private var registrations: [UUID: Registration] = [:]
    private var frameCount: UInt64 = 0
    private let lock = NSLock()
    private var dirty = true
    private var snapshot: ContiguousArray<(skipFrames: Int, callback: () -> Void)> = []

    private struct Registration {
        let skipFrames: Int      // Fire every N frames (1 = every frame, 2 = every other, etc.)
        let callback: () -> Void
    }

    private init() {}

    // MARK: - Public API

    /// Register a callback at the specified frequency.
    /// Returns a `TickToken` — hold it to keep the registration alive.
    /// Set it to `nil` (or let it deinit) to unregister.
    ///
    /// - Parameter hz: Desired callback frequency (1–60). Values above 60 clamp to 60.
    ///   Common values: 60 (every frame), 30 (every 2nd), 20 (every 3rd), 10 (every 6th).
    /// - Parameter callback: Called on the main thread at the requested cadence.
    /// - Returns: A `TickToken` that keeps the registration alive.
    func register(hz: Int, callback: @escaping () -> Void) -> TickToken {
        let id = UUID()
        let clampedHz = max(1, min(60, hz))
        let skip = max(1, 60 / clampedHz)

        lock.lock()
        registrations[id] = Registration(skipFrames: skip, callback: callback)
        dirty = true
        lock.unlock()

        ensureRunning()
        return TickToken(id: id, scheduler: self)
    }

    /// Remove a registration by ID. Called automatically by TickToken.deinit.
    func unregister(_ id: UUID) {
        lock.lock()
        registrations.removeValue(forKey: id)
        dirty = true
        let empty = registrations.isEmpty
        lock.unlock()

        if empty { stop() }
    }

    /// Number of active registrations (for diagnostics).
    var activeCount: Int {
        lock.lock()
        defer { lock.unlock() }
        return registrations.count
    }

    // MARK: - Internal

    private func ensureRunning() {
        guard timer == nil else { return }
        let t = Timer(timeInterval: 1.0 / 60.0, repeats: true) { [weak self] _ in
            self?.tick()
        }
        // .common mode: fires during scroll gestures, not just idle
        RunLoop.main.add(t, forMode: .common)
        timer = t
    }

    private func tick() {
        frameCount &+= 1 // Wrapping add — overflow-safe

        if dirty {
            lock.lock()
            snapshot = ContiguousArray(registrations.values.map { ($0.skipFrames, $0.callback) })
            dirty = false
            lock.unlock()
        }

        for reg in snapshot {
            if frameCount % UInt64(reg.skipFrames) == 0 {
                reg.callback()
            }
        }
    }

    private func stop() {
        timer?.invalidate()
        timer = nil
        frameCount = 0
    }
}

// MARK: - TickToken

/// Auto-cancelling handle for a TickScheduler registration.
/// Hold a strong reference to keep the callback alive.
/// Set to nil or let it deinit to unregister.
final class TickToken {
    fileprivate let id: UUID
    private weak var scheduler: TickScheduler?

    fileprivate init(id: UUID, scheduler: TickScheduler) {
        self.id = id
        self.scheduler = scheduler
    }

    /// Explicitly cancel the registration.
    func cancel() {
        scheduler?.unregister(id)
    }

    deinit {
        cancel()
    }
}
