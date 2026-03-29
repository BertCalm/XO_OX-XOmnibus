import Foundation
import MultipeerConnectivity

// MARK: - DiveSessionStatus

enum DiveSessionStatus: String, Codable, CaseIterable {
    case waiting     // Host advertised, no guest yet
    case syncing     // Peer connected, exchanging specimen data
    case active      // Both players diving together
    case paused      // Connection hiccup, attempting recovery
    case completed   // Dive ended cleanly
    case solo        // Fallback: connection dropped mid-dive, continuing alone
}

// MARK: - DiveSpecimen

/// Lightweight specimen data sent over the network. Contains only what the other player
/// needs to know — no private breeding history, no wallet data. Small enough to stay < 1KB.
struct DiveSpecimen: Codable, Identifiable, Equatable {
    let id: UUID
    let specimenId: UUID
    let subtypeId: String
    let displayName: String
    let ownerId: String        // Which player owns this specimen
    let generation: Int
    let voiceRole: String      // Role this specimen plays during the dive (e.g. "melody", "bass", "pad")

    static func == (lhs: DiveSpecimen, rhs: DiveSpecimen) -> Bool {
        lhs.specimenId == rhs.specimenId
    }
}

// MARK: - CollabDiveEvent

/// A logged event during a collaborative dive — stored in the shared dive log.
struct CollabDiveEvent: Codable, Identifiable {
    let id: UUID
    let timestamp: Date
    let type: CollabDiveEventType
    let actorId: String        // Which player triggered the event
    let detail: String         // Human-readable description

    enum CollabDiveEventType: String, Codable {
        case sessionStarted
        case peerConnected
        case peerDisconnected
        case specimensSynced
        case diveStarted
        case depthMilestone
        case catchOccurred
        case crossCouplingDiscovered
        case scoreUpdated
        case diveEnded
        case fallbackToSolo
    }
}

// MARK: - CollabDiveRecord

/// The permanent post-dive record stored in both players' journals.
struct CollabDiveRecord: Codable, Identifiable {
    let id: UUID
    let sessionId: String
    let date: Date
    let hostPlayerName: String
    let guestPlayerName: String
    let combinedScore: Int
    let bonusMultiplier: Float     // 1.5 for completed collab, 1.0 for solo fallback
    let finalScore: Int            // combinedScore × bonusMultiplier
    let maxDepthReached: Int
    let deepestZone: String
    let specimensUsed: [DiveSpecimen]
    let catchesDuringDive: [DiveSpecimen]   // New specimens both players received
    let crossCouplingDiscoveries: [CrossCouplingDiscovery]
    let durationSeconds: TimeInterval
    let completedCollaboratively: Bool     // false if one player dropped out

    var scoreString: String { "\(finalScore)" }
    var depthString: String { "\(maxDepthReached)m" }
}

// MARK: - CrossCouplingDiscovery

/// Records a coupling event between specimens belonging to different players.
struct CrossCouplingDiscovery: Codable, Identifiable {
    let id: UUID
    let hostSpecimenId: UUID
    let guestSpecimenId: UUID
    let hostSpecimenName: String
    let guestSpecimenName: String
    let couplingType: String        // e.g. "HARMONIC", "RHYTHM", "RESONANCE"
    let discoveredAt: Date
}

// MARK: - Network Message

/// A small JSON message sent between players via MultipeerConnectivity.
/// All message payloads are kept < 1KB to minimise latency on local Bonjour.
struct CollabNetworkMessage: Codable {
    let type: MessageType
    let senderId: String
    let timestamp: Date
    let payload: String            // JSON-encoded per-type payload (kept flat for speed)

    enum MessageType: String, Codable {
        case sync                  // Initial handshake — exchange player names
        case specimenData          // Send your 3 chosen specimens
        case depthUpdate           // Current depth broadcast
        case scoreUpdate           // Current combined-score proposal
        case catchEvent            // A new specimen was caught
        case couplingDiscovery     // Cross-player coupling detected
        case diveEnd               // Initiating graceful dive end
        case heartbeat             // Keep-alive ping every 10 seconds
    }
}

// MARK: - Collab Sync Payload

/// Payload for MessageType.sync — player introduction.
struct SyncPayload: Codable {
    let playerName: String
    let sessionId: String
}

/// Payload for MessageType.specimenData — transmits chosen specimens.
struct SpecimenDataPayload: Codable {
    let specimens: [DiveSpecimen]
}

/// Payload for MessageType.depthUpdate — current dive depth.
struct DepthUpdatePayload: Codable {
    let depth: Int
}

/// Payload for MessageType.scoreUpdate — agreed-upon running score.
struct ScoreUpdatePayload: Codable {
    let score: Int
}

/// Payload for MessageType.catchEvent — a new specimen both players should receive.
struct CatchEventPayload: Codable {
    let caught: DiveSpecimen
    let catcherId: String
}

/// Payload for MessageType.couplingDiscovery.
struct CouplingDiscoveryPayload: Codable {
    let discovery: CrossCouplingDiscovery
}

// MARK: - CollaborativeDiveManager

/// Manages peer-to-peer collaborative dives via MultipeerConnectivity.
///
/// Two players each bring 3 specimens (6 total in the dive). They advance through
/// the water column together, share score, and can catch specimens that both players
/// receive. Cross-player wiring is discovered when specimens from different owners
/// happen to couple during the dive.
///
/// Network protocol: JSON messages over MCSession. Host advertises; guest browses.
/// If the connection drops mid-dive, each player falls back to solo with their own 3 specimens.
/// Session times out after 5 minutes if no peer is found.
final class CollaborativeDiveManager: NSObject, ObservableObject {

    // MARK: - Published State

    @Published var sessionState: DiveSessionStatus = .waiting
    @Published var connectedPeer: String? = nil        // Other player's display name
    @Published var combinedScore: Int = 0
    @Published var currentDepth: Int = 0
    @Published var diveLog: [CollabDiveEvent] = []

    // MARK: - Session Data

    private(set) var sessionId: String = ""
    private(set) var hostSpecimens: [DiveSpecimen] = []      // This player's 3 specimens
    private(set) var guestSpecimens: [DiveSpecimen] = []     // Other player's 3 specimens
    private(set) var catches: [DiveSpecimen] = []
    private(set) var couplingDiscoveries: [CrossCouplingDiscovery] = []

    /// All specimens combined (used by DiveComposer when collab is active).
    var combinedSpecimens: [DiveSpecimen] {
        Array((hostSpecimens + guestSpecimens).prefix(6))
    }

    // MARK: - MultipeerConnectivity

    private let serviceType = "obrix-dive"
    private var peerId: MCPeerID
    private var session: MCSession
    private var advertiser: MCNearbyServiceAdvertiser?
    private var browser: MCNearbyServiceBrowser?
    private var connectedPeerID: MCPeerID?

    // MARK: - Players

    private let localPlayerId: String
    private var remotePlayerId: String = ""
    private var isHost: Bool = false
    private var diveStartDate: Date?

    // MARK: - Timeout

    /// 5-minute limit to find a peer before the session auto-cancels.
    private static let searchTimeoutSeconds: TimeInterval = 300
    private var searchTimer: Timer?

    // MARK: - Callbacks

    /// Called when a new specimen is caught and both players should receive it.
    var onCatchEvent: ((DiveSpecimen) -> Void)?
    /// Called when a cross-player coupling is discovered.
    var onCouplingDiscovery: ((CrossCouplingDiscovery) -> Void)?
    /// Called when the dive ends and a record is ready.
    var onDiveCompleted: ((CollabDiveRecord) -> Void)?
    /// Called when connection drops mid-dive and fallback kicks in.
    var onFallbackToSolo: (() -> Void)?
    /// Called when state changes (so UI can react).
    var onStateChange: ((DiveSessionStatus) -> Void)?

    // MARK: - Init

    init(playerName: String) {
        self.localPlayerId = playerName
        self.peerId = MCPeerID(displayName: playerName)
        self.session = MCSession(peer: peerId,
                                 securityIdentity: nil,
                                 encryptionPreference: .required)
        super.init()
        self.session.delegate = self
    }

    // MARK: - Host Flow

    /// Host flow — step 1: create a session and begin advertising.
    /// Call before presenting the "Waiting for partner..." UI.
    func createSession(withSpecimens specimens: [DiveSpecimen]) {
        precondition(specimens.count <= 3, "Maximum 3 specimens per player")
        isHost = true
        sessionId = UUID().uuidString
        hostSpecimens = specimens
        sessionState = .waiting
        onStateChange?(.waiting)
        logEvent(type: .sessionStarted, detail: "Host created session \(sessionId)")

        advertiser = MCNearbyServiceAdvertiser(peer: peerId,
                                               discoveryInfo: ["sessionId": sessionId],
                                               serviceType: serviceType)
        advertiser?.delegate = self
        advertiser?.startAdvertisingPeer()
        startSearchTimeout()
    }

    // MARK: - Guest Flow

    /// Guest flow — step 1: start browsing for nearby host sessions.
    /// Specimens must be provided upfront so the browser delegate has them when a peer is found.
    func browseForSessions(withSpecimens specimens: [DiveSpecimen]) {
        precondition(specimens.count <= 3, "Maximum 3 specimens per player")
        isHost = false
        guestSpecimens = specimens
        sessionState = .waiting
        onStateChange?(.waiting)

        browser = MCNearbyServiceBrowser(peer: peerId, serviceType: serviceType)
        browser?.delegate = self
        browser?.startBrowsingForPeers()
        startSearchTimeout()
    }

    /// Guest flow — step 2: join a specific host (called by browser delegate).
    func joinSession(host: MCPeerID, sessionId: String, withSpecimens specimens: [DiveSpecimen]) {
        precondition(specimens.count <= 3, "Maximum 3 specimens per player")
        guestSpecimens = specimens
        self.sessionId = sessionId
        browser?.invitePeer(host, to: session, withContext: nil, timeout: 30)
    }

    // MARK: - Shared Dive Control

    /// Called on both sides once specimen data is exchanged and the dive should begin.
    func startDive() {
        guard sessionState == .syncing else { return }
        sessionState = .active
        onStateChange?(.active)
        diveStartDate = Date()
        logEvent(type: .diveStarted, detail: "Dive started with \(combinedSpecimens.count) specimens")

        if isHost {
            send(type: .specimenData, payload: SpecimenDataPayload(specimens: hostSpecimens))
        }
    }

    /// Update the current depth. Host broadcasts to guest; both UIs stay in sync.
    func updateDepth(_ depth: Int) {
        guard sessionState == .active else { return }
        currentDepth = depth

        let milestone = depth > 0 && depth % 100 == 0
        if milestone {
            logEvent(type: .depthMilestone, detail: "Reached \(depth)m")
        }

        if isHost {
            send(type: .depthUpdate, payload: DepthUpdatePayload(depth: depth))
        }
    }

    /// Update the shared score. Both players contribute to one pool.
    func addScore(_ points: Int) {
        guard sessionState == .active else { return }
        combinedScore += points
        if isHost {
            send(type: .scoreUpdate, payload: ScoreUpdatePayload(score: combinedScore))
        }
    }

    /// Record a catch. Both players receive the specimen as a collaboration bonus.
    func recordCatch(_ specimen: DiveSpecimen) {
        guard sessionState == .active else { return }
        catches.append(specimen)
        logEvent(type: .catchOccurred,
                 detail: "\(specimen.displayName) caught by \(specimen.ownerId)")
        onCatchEvent?(specimen)
        send(type: .catchEvent,
             payload: CatchEventPayload(caught: specimen, catcherId: localPlayerId))
    }

    /// Record a cross-player coupling discovery.
    func recordCrossCoupling(hostSpecimen: DiveSpecimen,
                             guestSpecimen: DiveSpecimen,
                             couplingType: String) {
        guard sessionState == .active else { return }
        let discovery = CrossCouplingDiscovery(
            id: UUID(),
            hostSpecimenId: hostSpecimen.specimenId,
            guestSpecimenId: guestSpecimen.specimenId,
            hostSpecimenName: hostSpecimen.displayName,
            guestSpecimenName: guestSpecimen.displayName,
            couplingType: couplingType,
            discoveredAt: Date()
        )
        couplingDiscoveries.append(discovery)
        logEvent(type: .crossCouplingDiscovered,
                 detail: "\(hostSpecimen.displayName) ↔ \(guestSpecimen.displayName) (\(couplingType))")
        onCouplingDiscovery?(discovery)
        send(type: .couplingDiscovery, payload: CouplingDiscoveryPayload(discovery: discovery))
    }

    /// End the dive and produce a CollabDiveRecord for both players.
    func endDive(deepestZone: String) {
        guard sessionState == .active || sessionState == .paused else { return }
        send(type: .diveEnd, payload: SyncPayload(playerName: localPlayerId, sessionId: sessionId))
        finaliseDive(deepestZone: deepestZone, collaborative: connectedPeer != nil)
    }

    // MARK: - Teardown

    func disconnect() {
        searchTimer?.invalidate()
        searchTimer = nil
        advertiser?.stopAdvertisingPeer()
        browser?.stopBrowsingForPeers()
        session.disconnect()
        connectedPeerID = nil
        connectedPeer = nil
    }

    // MARK: - Private Helpers

    private func startSearchTimeout() {
        searchTimer?.invalidate()
        searchTimer = Timer.scheduledTimer(
            withTimeInterval: Self.searchTimeoutSeconds,
            repeats: false
        ) { [weak self] _ in
            guard let self = self, self.sessionState == .waiting else { return }
            self.cancelSearch()
        }
    }

    private func cancelSearch() {
        searchTimer?.invalidate()
        searchTimer = nil
        advertiser?.stopAdvertisingPeer()
        browser?.stopBrowsingForPeers()
        sessionState = .solo
        onStateChange?(.solo)
        logEvent(type: .fallbackToSolo, detail: "Search timed out after 5 minutes")
    }

    private func handleConnectionDrop() {
        guard sessionState == .active else { return }
        sessionState = .solo
        onStateChange?(.solo)
        logEvent(type: .fallbackToSolo, detail: "Connection dropped mid-dive — continuing solo")
        onFallbackToSolo?()
        // Guest's 3 specimens are already in guestSpecimens; host keeps hostSpecimens.
        // DiveComposer should be reconfigured with only the local player's specimens.
    }

    private func finaliseDive(deepestZone: String, collaborative: Bool) {
        sessionState = .completed
        onStateChange?(.completed)

        let duration = diveStartDate.map { Date().timeIntervalSince($0) } ?? 0
        let multiplier: Float = collaborative ? 1.5 : 1.0
        let final = Int(Float(combinedScore) * multiplier)

        logEvent(type: .diveEnded,
                 detail: "Final score \(final) (\(combinedScore) × \(multiplier))")

        let record = CollabDiveRecord(
            id: UUID(),
            sessionId: sessionId,
            date: Date(),
            hostPlayerName: isHost ? localPlayerId : remotePlayerId,
            guestPlayerName: isHost ? remotePlayerId : localPlayerId,
            combinedScore: combinedScore,
            bonusMultiplier: multiplier,
            finalScore: final,
            maxDepthReached: currentDepth,
            deepestZone: deepestZone,
            specimensUsed: combinedSpecimens,
            catchesDuringDive: catches,
            crossCouplingDiscoveries: couplingDiscoveries,
            durationSeconds: duration,
            completedCollaboratively: collaborative
        )
        onDiveCompleted?(record)
        disconnect()
    }

    // MARK: - Networking

    private func send<T: Codable>(type: CollabNetworkMessage.MessageType, payload: T) {
        guard let peer = connectedPeerID else { return }
        guard let payloadData = try? JSONEncoder().encode(payload),
              let payloadString = String(data: payloadData, encoding: .utf8) else { return }

        let message = CollabNetworkMessage(
            type: type,
            senderId: localPlayerId,
            timestamp: Date(),
            payload: payloadString
        )
        guard let data = try? JSONEncoder().encode(message) else { return }

        try? session.send(data, toPeers: [peer], with: .reliable)
    }

    private func receive(data: Data) {
        guard let message = try? JSONDecoder().decode(CollabNetworkMessage.self, from: data) else { return }
        handleMessage(message)
    }

    private func handleMessage(_ message: CollabNetworkMessage) {
        // MCSessionDelegate callbacks arrive on a private MPC background queue.
        // All state mutations must happen on the main thread.
        DispatchQueue.main.async { [weak self] in
            guard let self else { return }
            switch message.type {

            case .sync:
                guard let p = self.decode(SyncPayload.self, from: message.payload) else { return }
                self.remotePlayerId = p.playerName
                if !self.isHost { self.sessionId = p.sessionId }
                self.connectedPeer = p.playerName
                self.logEvent(type: .peerConnected, detail: "\(p.playerName) joined")
                self.sessionState = .syncing
                self.onStateChange?(.syncing)
                // Guest sends their specimens immediately upon receiving sync from host
                if !self.isHost {
                    self.send(type: .specimenData,
                              payload: SpecimenDataPayload(specimens: self.guestSpecimens))
                } else {
                    // Host acknowledges and sends own specimens
                    self.send(type: .specimenData,
                              payload: SpecimenDataPayload(specimens: self.hostSpecimens))
                    self.send(type: .sync,
                              payload: SyncPayload(playerName: self.localPlayerId, sessionId: self.sessionId))
                }

            case .specimenData:
                guard let p = self.decode(SpecimenDataPayload.self, from: message.payload) else { return }
                if self.isHost {
                    self.guestSpecimens = Array(p.specimens.prefix(3))
                } else {
                    self.hostSpecimens = Array(p.specimens.prefix(3))
                }
                self.logEvent(type: .specimensSynced,
                              detail: "\(p.specimens.count) specimens received from \(message.senderId)")
                // Once both sides have exchanged specimens, mark ready to start
                if !self.hostSpecimens.isEmpty && !self.guestSpecimens.isEmpty {
                    self.sessionState = .syncing
                    self.onStateChange?(.syncing)
                }

            case .depthUpdate:
                guard let p = self.decode(DepthUpdatePayload.self, from: message.payload) else { return }
                self.currentDepth = p.depth

            case .scoreUpdate:
                guard let p = self.decode(ScoreUpdatePayload.self, from: message.payload) else { return }
                self.combinedScore = p.score

            case .catchEvent:
                guard let p = self.decode(CatchEventPayload.self, from: message.payload) else { return }
                self.catches.append(p.caught)
                self.onCatchEvent?(p.caught)

            case .couplingDiscovery:
                guard let p = self.decode(CouplingDiscoveryPayload.self, from: message.payload) else { return }
                self.couplingDiscoveries.append(p.discovery)
                self.onCouplingDiscovery?(p.discovery)

            case .diveEnd:
                self.finaliseDive(deepestZone: "Unknown", collaborative: true)

            case .heartbeat:
                break  // Acknowledged silently
            }
        }
    }

    private func decode<T: Decodable>(_ type: T.Type, from string: String) -> T? {
        guard let data = string.data(using: .utf8) else { return nil }
        return try? JSONDecoder().decode(type, from: data)
    }

    private func logEvent(type: CollabDiveEvent.CollabDiveEventType, detail: String) {
        let event = CollabDiveEvent(id: UUID(),
                                    timestamp: Date(),
                                    type: type,
                                    actorId: localPlayerId,
                                    detail: detail)
        DispatchQueue.main.async { self.diveLog.append(event) }
    }
}

// MARK: - MCSessionDelegate

extension CollaborativeDiveManager: MCSessionDelegate {

    func session(_ session: MCSession,
                 peer peerID: MCPeerID,
                 didChange state: MCSessionState) {
        switch state {
        case .connected:
            DispatchQueue.main.async {
                self.connectedPeerID = peerID
                self.searchTimer?.invalidate()
                self.searchTimer = nil
                self.advertiser?.stopAdvertisingPeer()
                self.browser?.stopBrowsingForPeers()
                // Send sync introduction
                self.send(type: .sync,
                          payload: SyncPayload(playerName: self.localPlayerId,
                                               sessionId: self.sessionId))
            }

        case .notConnected:
            DispatchQueue.main.async {
                if self.sessionState == .active || self.sessionState == .paused {
                    self.handleConnectionDrop()
                } else if self.sessionState == .syncing {
                    self.sessionState = .waiting
                    self.onStateChange?(.waiting)
                }
                self.connectedPeerID = nil
                self.connectedPeer = nil
                self.logEvent(type: .peerDisconnected, detail: "\(peerID.displayName) disconnected")
            }

        case .connecting:
            break

        @unknown default:
            break
        }
    }

    func session(_ session: MCSession, didReceive data: Data, fromPeer peerID: MCPeerID) {
        receive(data: data)
    }

    func session(_ session: MCSession,
                 didReceive stream: InputStream,
                 withName streamName: String,
                 fromPeer peerID: MCPeerID) {
        // Not used
    }

    func session(_ session: MCSession,
                 didStartReceivingResourceWithName resourceName: String,
                 fromPeer peerID: MCPeerID,
                 with progress: Progress) {
        // Not used
    }

    func session(_ session: MCSession,
                 didFinishReceivingResourceWithName resourceName: String,
                 fromPeer peerID: MCPeerID,
                 at localURL: URL?,
                 withError error: Error?) {
        // Not used
    }
}

// MARK: - MCNearbyServiceAdvertiserDelegate

extension CollaborativeDiveManager: MCNearbyServiceAdvertiserDelegate {

    func advertiser(_ advertiser: MCNearbyServiceAdvertiser,
                    didReceiveInvitationFromPeer peerID: MCPeerID,
                    withContext context: Data?,
                    invitationHandler: @escaping (Bool, MCSession?) -> Void) {
        // Auto-accept the first incoming invitation while in waiting state
        guard sessionState == .waiting else {
            invitationHandler(false, nil)
            return
        }
        invitationHandler(true, session)
    }

    func advertiser(_ advertiser: MCNearbyServiceAdvertiser,
                    didNotStartAdvertisingPeer error: Error) {
        logEvent(type: .fallbackToSolo,
                 detail: "Advertiser failed: \(error.localizedDescription)")
    }
}

// MARK: - MCNearbyServiceBrowserDelegate

extension CollaborativeDiveManager: MCNearbyServiceBrowserDelegate {

    func browser(_ browser: MCNearbyServiceBrowser,
                 foundPeer peerID: MCPeerID,
                 withDiscoveryInfo info: [String: String]?) {
        // Auto-invite first found host (guests bring their specimens before browsing)
        guard sessionState == .waiting else { return }
        let foundSessionId = info?["sessionId"] ?? ""
        joinSession(host: peerID, sessionId: foundSessionId, withSpecimens: guestSpecimens)
    }

    func browser(_ browser: MCNearbyServiceBrowser, lostPeer peerID: MCPeerID) {
        // Peer disappeared while browsing — keep browsing unless timed out
    }

    func browser(_ browser: MCNearbyServiceBrowser, didNotStartBrowsingForPeers error: Error) {
        logEvent(type: .fallbackToSolo,
                 detail: "Browser failed: \(error.localizedDescription)")
    }
}

// MARK: - Achievement Triggers

extension CollaborativeDiveManager {

    /// Achievement IDs triggered by collaborative dives.
    enum CollabAchievement: String {
        case firstCollabDive     = "collab_first"
        case tenCollabDives      = "collab_ten"
        case firstCrossCoupling  = "collab_cross_coupling_first"
    }

    /// Check achievements after a dive completes. Call from onDiveCompleted handler.
    static func checkAchievements(record: CollabDiveRecord, totalCollabDives: Int) -> [CollabAchievement] {
        var unlocked: [CollabAchievement] = []
        if totalCollabDives == 1 { unlocked.append(.firstCollabDive) }
        if totalCollabDives == 10 { unlocked.append(.tenCollabDives) }
        if !record.crossCouplingDiscoveries.isEmpty { unlocked.append(.firstCrossCoupling) }
        return unlocked
    }
}
