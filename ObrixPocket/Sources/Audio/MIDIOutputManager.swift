import Foundation
import CoreMIDI

/// Manages CoreMIDI virtual source for MIDI output from the reef.
/// Other apps (AUM, GarageBand, etc.) see "OBRIX Pocket" as a MIDI source.
final class MIDIOutputManager: ObservableObject {
    @Published var isEnabled = false
    @Published var connectedDestinations = 0

    private var midiClient = MIDIClientRef()
    private var midiSource = MIDIEndpointRef()
    private var isSetup = false

    init() {
        setupMIDI()
    }

    deinit {
        if isSetup {
            MIDIEndpointDispose(midiSource)
            MIDIClientDispose(midiClient)
        }
    }

    // MARK: - Setup

    private func setupMIDI() {
        var status = MIDIClientCreateWithBlock("OBRIX Pocket" as CFString, &midiClient) { [weak self] notification in
            // Handle MIDI setup changes
            self?.updateConnectionCount()
        }

        guard status == noErr else {
            print("[MIDIOutput] Client creation failed: \(status)")
            return
        }

        status = MIDISourceCreateWithProtocol(
            midiClient,
            "OBRIX Pocket" as CFString,
            ._1_0, // MIDI 1.0 protocol
            &midiSource
        )

        guard status == noErr else {
            print("[MIDIOutput] Source creation failed: \(status)")
            return
        }

        isSetup = true
        isEnabled = true
        updateConnectionCount()
    }

    // MARK: - Send Events

    /// Send note-on from a reef slot
    func sendNoteOn(note: UInt8, velocity: UInt8, channel: UInt8 = 0) {
        guard isSetup, isEnabled else { return }

        let message: [UInt8] = [0x90 | (channel & 0x0F), note & 0x7F, velocity & 0x7F]
        sendBytes(message)
    }

    /// Send note-off from a reef slot
    func sendNoteOff(note: UInt8, channel: UInt8 = 0) {
        guard isSetup, isEnabled else { return }

        let message: [UInt8] = [0x80 | (channel & 0x0F), note & 0x7F, 0]
        sendBytes(message)
    }

    /// Send CC (for macro control, coupling depth, etc.)
    func sendCC(controller: UInt8, value: UInt8, channel: UInt8 = 0) {
        guard isSetup, isEnabled else { return }

        let message: [UInt8] = [0xB0 | (channel & 0x0F), controller & 0x7F, value & 0x7F]
        sendBytes(message)
    }

    // MARK: - Private

    private func sendBytes(_ bytes: [UInt8]) {
        // MIDI 1.0 UMP (Universal MIDI Packet) for MIDISourceCreateWithProtocol
        var packet = MIDIEventPacket()
        packet.timeStamp = mach_absolute_time()
        packet.wordCount = 1

        // Pack MIDI 1.0 message into UMP format
        let word: UInt32 = (0x20 << 24) | // Message type: MIDI 1.0 channel voice
                           (UInt32(bytes[0]) << 16) |
                           (UInt32(bytes.count > 1 ? bytes[1] : 0) << 8) |
                           UInt32(bytes.count > 2 ? bytes[2] : 0)

        withUnsafeMutablePointer(to: &packet.words) { wordsPtr in
            wordsPtr.withMemoryRebound(to: UInt32.self, capacity: 1) { ptr in
                ptr.pointee = word
            }
        }

        var packetList = MIDIEventList()
        packetList.protocol = ._1_0
        packetList.numPackets = 1

        withUnsafeMutablePointer(to: &packetList.packet) { ptr in
            ptr.pointee = packet
        }

        MIDIReceivedEventList(midiSource, &packetList)
    }

    private func updateConnectionCount() {
        connectedDestinations = MIDIGetNumberOfDestinations()
    }
}
