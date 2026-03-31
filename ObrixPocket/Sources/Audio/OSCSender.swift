import Foundation
import Network

/// Sends OSC messages over UDP for wireless instrument control.
/// Target: desktop XOceanus app listening on port 9000.
final class OSCSender: ObservableObject {
    @Published var isConnected = false
    @Published var targetHost: String = "" {
        didSet { UserDefaults.standard.set(targetHost, forKey: "obrix_osc_host") }
    }
    @Published var targetPort: UInt16 = 9000

    static let shared = OSCSender()

    private var connection: NWConnection?

    init() {
        targetHost = UserDefaults.standard.string(forKey: "obrix_osc_host") ?? ""
    }

    /// Connect to the target host
    func connect() {
        guard !targetHost.isEmpty else { return }

        let host = NWEndpoint.Host(targetHost)
        guard let port = NWEndpoint.Port(rawValue: targetPort) else { return }

        connection = NWConnection(host: host, port: port, using: .udp)
        connection?.stateUpdateHandler = { [weak self] state in
            DispatchQueue.main.async {
                self?.isConnected = (state == .ready)
            }
        }
        connection?.start(queue: .global(qos: .userInteractive))
    }

    func disconnect() {
        connection?.cancel()
        connection = nil
        isConnected = false
    }

    // MARK: - OSC Messages

    /// Send note on: /obrix/note/on [note] [velocity]
    func sendNoteOn(note: Int, velocity: Float) {
        send(address: "/obrix/note/on", args: [.int(Int32(note)), .float(velocity)])
    }

    /// Send note off: /obrix/note/off [note]
    func sendNoteOff(note: Int) {
        send(address: "/obrix/note/off", args: [.int(Int32(note))])
    }

    /// Send expression: /obrix/expression [filterMod] [pitchBend]
    func sendExpression(filterMod: Float, pitchBend: Float) {
        send(address: "/obrix/expression", args: [.float(filterMod), .float(pitchBend)])
    }

    /// Send parameter change: /obrix/param [paramId] [value]
    func sendParam(paramId: String, value: Float) {
        send(address: "/obrix/param", args: [.string(paramId), .float(value)])
    }

    // MARK: - OSC Encoding

    enum OSCArg {
        case int(Int32)
        case float(Float)
        case string(String)
    }

    private func send(address: String, args: [OSCArg]) {
        guard let connection, isConnected else { return }

        var data = Data()

        // Address pattern (null-terminated, padded to 4 bytes)
        data.append(oscString(address))

        // Type tag string
        var typeTag = ","
        for arg in args {
            switch arg {
            case .int:    typeTag += "i"
            case .float:  typeTag += "f"
            case .string: typeTag += "s"
            }
        }
        data.append(oscString(typeTag))

        // Arguments
        for arg in args {
            switch arg {
            case .int(let v):
                var bigEndian = v.bigEndian
                data.append(Data(bytes: &bigEndian, count: 4))
            case .float(let v):
                var bits = v.bitPattern.bigEndian
                data.append(Data(bytes: &bits, count: 4))
            case .string(let v):
                data.append(oscString(v))
            }
        }

        connection.send(content: data, completion: .idempotent)
    }

    private func oscString(_ string: String) -> Data {
        var data = string.data(using: .utf8) ?? Data()
        data.append(0) // Null terminator
        // Pad to 4-byte boundary
        while data.count % 4 != 0 { data.append(0) }
        return data
    }
}
