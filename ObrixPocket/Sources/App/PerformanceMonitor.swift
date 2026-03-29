import Foundation

/// Monitors memory usage, frame drops, and audio buffer underruns.
/// Reports to console in debug builds. Silent in release.
final class PerformanceMonitor {

    static let shared = PerformanceMonitor()

    /// Current memory usage in MB
    var memoryUsageMB: Float {
        var info = mach_task_basic_info()
        var count = mach_msg_type_number_t(MemoryLayout<mach_task_basic_info>.size) / 4
        let result = withUnsafeMutablePointer(to: &info) {
            $0.withMemoryRebound(to: integer_t.self, capacity: 1) {
                task_info(mach_task_self_, task_flavor_t(MACH_TASK_BASIC_INFO), $0, &count)
            }
        }
        return result == KERN_SUCCESS ? Float(info.resident_size) / (1024 * 1024) : 0
    }

    /// Check if we're approaching memory pressure (> 150MB for an iPhone game)
    var isMemoryPressure: Bool {
        memoryUsageMB > 150
    }

    /// Audio buffer underrun counter (incremented by audio engine)
    var bufferUnderruns: Int = 0

    /// Log current state (debug only)
    func logState() {
        #if DEBUG
        let mem = String(format: "%.1f", memoryUsageMB)
        print("[Perf] Memory: \(mem)MB | Underruns: \(bufferUnderruns)")
        #endif
    }

    /// Start periodic monitoring (every 30 seconds)
    func startMonitoring() {
        #if DEBUG
        Timer.scheduledTimer(withTimeInterval: 30, repeats: true) { [weak self] _ in
            self?.logState()
        }
        #endif
    }
}
