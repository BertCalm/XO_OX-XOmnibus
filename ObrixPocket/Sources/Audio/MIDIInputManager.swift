import Foundation
import CoreMIDI

/// Receives MIDI input from external keyboards/controllers and routes to ObrixBridge.
final class MIDIInputManager: ObservableObject {
    @Published var isEnabled = false
    @Published var connectedSources = 0
    @Published var lastNoteReceived: Int?

    private var midiClient = MIDIClientRef()
    private var midiPort = MIDIPortRef()
    private var isSetup = false

    // Callbacks
    var onNoteOn: ((Int, Float) -> Void)?
    var onNoteOff: ((Int) -> Void)?
    var onCC: ((Int, Int) -> Void)?  // (controller, value)

    init() {
        setupMIDI()
    }

    deinit {
        if isSetup {
            MIDIPortDispose(midiPort)
            MIDIClientDispose(midiClient)
        }
    }

    private func setupMIDI() {
        var status = MIDIClientCreateWithBlock("OBRIX Pocket Input" as CFString, &midiClient) { [weak self] _ in
            self?.updateSourceCount()
        }
        guard status == noErr else {
            print("[MIDIInput] Client creation failed: \(status)")
            return
        }

        status = MIDIInputPortCreateWithProtocol(
            midiClient,
            "OBRIX Input" as CFString,
            ._1_0,
            &midiPort
        ) { [weak self] eventList, _ in
            self?.handleMIDI(eventList)
        }
        guard status == noErr else {
            print("[MIDIInput] Port creation failed: \(status)")
            return
        }

        // Connect to all existing sources
        connectAllSources()
        isSetup = true
        isEnabled = true
        updateSourceCount()
    }

    private func connectAllSources() {
        let sourceCount = MIDIGetNumberOfSources()
        for i in 0..<sourceCount {
            let source = MIDIGetSource(i)
            MIDIPortConnectSource(midiPort, source, nil)
        }
    }

    private func handleMIDI(_ eventList: UnsafePointer<MIDIEventList>) {
        let list = eventList.pointee
        var packet = list.packet

        for _ in 0..<list.numPackets {
            // Parse UMP (Universal MIDI Packet)
            let word = packet.words.0
            let messageType = (word >> 28) & 0xF

            if messageType == 0x2 { // MIDI 1.0 Channel Voice
                let status = (word >> 16) & 0xFF
                let data1 = (word >> 8) & 0x7F
                let data2 = word & 0x7F

                let msgType = status & 0xF0
                let note = Int(data1)
                let velocity = Float(data2) / 127.0

                DispatchQueue.main.async { [weak self] in
                    switch msgType {
                    case 0x90: // Note On
                        if velocity > 0 {
                            self?.lastNoteReceived = note
                            self?.onNoteOn?(note, velocity)
                        } else {
                            self?.onNoteOff?(note)
                        }
                    case 0x80: // Note Off
                        self?.onNoteOff?(note)
                    case 0xB0: // CC
                        self?.onCC?(note, Int(data2))
                    default:
                        break
                    }
                }
            }

            packet = MIDIEventPacketNext(&packet).pointee
        }
    }

    private func updateSourceCount() {
        DispatchQueue.main.async { [weak self] in
            self?.connectedSources = MIDIGetNumberOfSources()
        }
    }
}
