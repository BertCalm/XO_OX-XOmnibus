import Foundation
import AVFoundation
import Combine
import UIKit

/// Manages the JUCE audio hosting layer for ObrixEngine.
/// Phase 0: Initializes audio session, creates JUCE bridge, routes touch events to notes.
final class AudioEngineManager: ObservableObject {
    @Published var isRunning = false
    @Published var cpuLoad: Float = 0.0
    @Published var hasPlayedFirstNote = false

    private var audioSessionConfigured = false
    private let midiOutput = MIDIOutputManager()

    /// Bridge to genetics, aging, memory, and nursery modifiers.
    /// Set after init by the app entry point once GameCoordinator is available.
    weak var gameCoordinator: GameCoordinator?

    /// Aging manager — provides LifeStage per slot for bridge queries.
    /// Set after init alongside gameCoordinator.
    weak var specimenAgingManager: SpecimenAgingManager?

    init() {
        configureAudioSession()

        // Crash recovery: deactivate any stale audio session from a prior crash.
        // If a previous run crashed while audio was active, the session may still
        // be holding the audio route in a bad state (causing distortion in other apps).
        do {
            try AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
        } catch {
            // Expected on first launch — no session was active
        }

        // Memory pressure handling — flush non-essential caches before jetsam kills the process.
        NotificationCenter.default.addObserver(
            forName: UIApplication.didReceiveMemoryWarningNotification,
            object: nil,
            queue: .main
        ) { [weak self] _ in
            self?.handleMemoryWarning()
        }
    }

    private func handleMemoryWarning() {
        print("[ObrixPocket] Memory warning received — flushing caches")
        // Clear non-essential caches
        slotParamCache.removeAll()
    }

    func start() {
        guard !isRunning else { return }

        // Activate audio session just before starting the bridge
        do {
            try AVAudioSession.sharedInstance().setActive(true)
        } catch {
            print("[ObrixPocket] Failed to activate audio session: \(error)")
            // Don't proceed with audio if session activation failed
            return
        }

        // Phase 0: Start JUCE audio bridge
        ObrixBridge.shared()?.startAudio()
        isRunning = true
    }

    func stop() {
        ObrixBridge.shared()?.stopAudio()
        isRunning = false
    }

    // MARK: - Reef Configuration (push wired specimen params to engine)

    /// Apply the full reef config on launch (sets defaults) and prime the param cache.
    func applyReefConfiguration(_ reefStore: ReefStore) {
        // Build the per-slot cache for all wired slots first
        rebuildParamCache(reefStore: reefStore)
        // Then configure engine with the first occupied slot's chain
        if let firstOccupied = reefStore.specimens.firstIndex(where: { $0 != nil }) {
            applySlotChain(slotIndex: firstOccupied, reefStore: reefStore)
        }
    }

    /// Configure the OBRIX engine for a specific slot's wired chain.
    /// Called when a slot is tapped — reads the specimen + its wired connections.
    func applySlotChain(slotIndex: Int, reefStore: ReefStore) {
        guard let bridge = ObrixBridge.shared(),
              let specimen = reefStore.specimens[slotIndex] else { return }

        // Step 1: Reset engine to defaults (turn off unused modules) — immediate write
        bridge.setParameterImmediate("obrix_src1Type", value: 0)  // Off
        bridge.setParameterImmediate("obrix_src2Type", value: 0)  // Off
        bridge.setParameterImmediate("obrix_proc1Type", value: 0) // Off
        bridge.setParameterImmediate("obrix_proc2Type", value: 0) // Off
        bridge.setParameterImmediate("obrix_proc3Type", value: 0) // Off
        bridge.setParameterImmediate("obrix_fx1Type", value: 0)   // Off
        bridge.setParameterImmediate("obrix_fx2Type", value: 0)   // Off
        bridge.setParameterImmediate("obrix_fx3Type", value: 0)   // Off
        bridge.setParameterImmediate("obrix_mod1Depth", value: 0)
        bridge.setParameterImmediate("obrix_mod2Depth", value: 0)
        // Ensure envelope is audible
        bridge.setParameterImmediate("obrix_ampAttack", value: 0.01)
        bridge.setParameterImmediate("obrix_ampDecay", value: 0.3)
        bridge.setParameterImmediate("obrix_ampSustain", value: 0.7)
        bridge.setParameterImmediate("obrix_ampRelease", value: 0.5)

        // Step 2: Apply the tapped specimen
        applySpecimenToEngine(specimen, bridge: bridge)

        // Step 3: Walk wired connections and apply them too
        let wiredSpecimens = findWiredChain(from: slotIndex, reefStore: reefStore)
        for wired in wiredSpecimens {
            applySpecimenToEngine(wired, bridge: bridge)
        }
    }

    /// Find all specimens wired to/from a given slot (one level deep).
    private func findWiredChain(from slotIndex: Int, reefStore: ReefStore) -> [Specimen] {
        guard let specimen = reefStore.specimens[slotIndex] else { return [] }

        var chain: [Specimen] = []
        for route in reefStore.couplingRoutes {
            // Find specimens connected to this one (either direction)
            if route.sourceId == specimen.id {
                if let idx = reefStore.specimens.firstIndex(where: { $0?.id == route.destId }),
                   let connected = reefStore.specimens[idx] {
                    chain.append(connected)
                }
            } else if route.destId == specimen.id {
                if let idx = reefStore.specimens.firstIndex(where: { $0?.id == route.sourceId }),
                   let connected = reefStore.specimens[idx] {
                    chain.append(connected)
                }
            }
        }
        return chain
    }

    // MARK: - Parameter Cache Build

    /// Build (or rebuild) the per-slot parameter cache from the current reef wiring.
    /// Call this on app launch and whenever wiring changes.
    func rebuildParamCache(reefStore: ReefStore) {
        slotParamCache.removeAll()
        for slotIndex in 0..<ReefStore.maxSlots {
            guard let specimen = reefStore.specimens[slotIndex] else { continue }
            var params: [(String, Float)] = []
            collectMappedParams(specimen, into: &params)
            let wired = findWiredChain(from: slotIndex, reefStore: reefStore)
            for w in wired {
                collectMappedParams(w, into: &params)
            }
            slotParamCache[slotIndex] = params
        }
    }

    /// Blast cached params for a slot without walking the full wiring chain.
    /// I-08: Used by the PlayKeyboard which calls ObrixBridge directly (bypassing noteOn).
    /// Replaces the per-keypress applySlotChain call in ReefTab.
    func applyCachedParams(for slotIndex: Int) {
        guard let bridge = ObrixBridge.shared(),
              let cached = slotParamCache[slotIndex] else { return }
        for (param, val) in Self.resetParams {
            bridge.setParameterImmediate(param, value: val)
        }
        for (param, val) in cached {
            bridge.setParameterImmediate(param, value: val)
        }
    }

    /// Collect all engine-mapped params for a single specimen into a flat array.
    /// Mirrors the category-switch logic in applySpecimenToEngine.
    private func collectMappedParams(_ specimen: Specimen, into params: inout [(String, Float)]) {
        // Module-type activation (same mapping as applySpecimenToEngine)
        switch specimen.category {
        case .source:
            let srcTypeMap: [String: Float] = [
                "polyblep-saw": 2, "polyblep-square": 3, "polyblep-tri": 4,
                "noise-white": 5, "noise-pink": 5,
                "wt-analog": 6, "wt-vocal": 6, "fm-basic": 7
            ]
            params.append(("obrix_src1Type", srcTypeMap[specimen.subtype] ?? 2))
        case .processor:
            let procTypeMap: [String: Float] = [
                "svf-lp": 1, "svf-hp": 2, "svf-bp": 3,
                "shaper-soft": 4, "shaper-hard": 4, "feedback": 5
            ]
            params.append(("obrix_proc1Type", procTypeMap[specimen.subtype] ?? 1))
        case .modulator:
            params.append(("obrix_mod2Type", 2))
            params.append(("obrix_mod2Target", 2))
        case .effect:
            let fxTypeMap: [String: Float] = [
                "delay-stereo": 1, "chorus-lush": 2, "reverb-hall": 3, "dist-warm": 1
            ]
            params.append(("obrix_fx1Type", fxTypeMap[specimen.subtype] ?? 1))
        }
        // Mapped parameter values — apply rarity multiplier to continuous params
        let rMult = Self.rarityMultiplier(specimen.rarity)

        // Build a mutable param dict for bridge-modifier overlay
        var paramDict: [String: Float] = [:]
        for (specimenParam, value) in specimen.parameterState {
            if let mapping = Self.parameterMapping[specimenParam] {
                let scaled = mapping.scale(value)
                paramDict[mapping.engineParam] = mapping.isContinuous ? (scaled * rMult) : scaled
            }
        }
        // Fill any gaps with catalog defaults so every specimen type's character
        // comes through even when parameterState was created before defaultParams existed.
        if let entry = SpecimenCatalog.entry(for: specimen.subtype) {
            for (key, defaultVal) in entry.defaultParams {
                if !specimen.parameterState.keys.contains(key) {
                    if let mapping = Self.parameterMapping[key] {
                        paramDict[mapping.engineParam] = mapping.scale(defaultVal)
                    }
                }
            }
        }

        // Apply bridge modifiers (genetics + aging + memory + nursery) on top of base params.
        //
        // The bridge builds its dict starting from zero: genetics first (additive offsets),
        // then AgingModifiers.apply (multiplicative on those genetic offsets), then memory
        // and nursery (more additive offsets). The result is a small delta dict — not
        // absolute final values.
        //
        // Merge strategy: add bridge deltas on top of the catalog-resolved base values.
        // For keys that AgingModifiers touches (cutoff, resonance, lfoDepth, attack,
        // release, tune), the aging multiplier was already baked into the bridge value —
        // so we apply these as additive offsets and trust the bridge for their sign.
        if let gc = gameCoordinator {
            let lifeStage = specimenAgingManager?.stage(for: slotIndex(of: specimen)) ?? .adult
            let playAge   = SpecimenAge.from(playSeconds: specimen.totalPlaySeconds)
            let bridgeMods = gc.effectiveAudioParameters(
                specimenId: specimen.id,
                subtypeId:  specimen.subtype,
                lifeStage:  lifeStage,
                playAge:    playAge
            )
            for (key, delta) in bridgeMods {
                paramDict[key] = (paramDict[key] ?? 0) + delta
            }
        }

        // Flatten the dict back into the output array
        for (engineParam, finalValue) in paramDict {
            params.append((engineParam, finalValue))
        }
    }

    /// Return the reef slot index for a specimen, or -1 if not found.
    /// Used by collectMappedParams to look up the aging stage.
    private func slotIndex(of specimen: Specimen) -> Int {
        guard let rs = reefStoreRef else { return -1 }
        return rs.specimens.firstIndex(where: { $0?.id == specimen.id }) ?? -1
    }

    /// Push a single specimen's parameters to the engine based on its category.
    /// Uses setParameterImmediate for synchronous writes (no queue delay).
    private func applySpecimenToEngine(_ specimen: Specimen, bridge: ObrixBridge) {
        // First: enable the correct module type based on specimen category
        switch specimen.category {
        case .source:
            // Map specimen source type to engine source type
            let srcTypeMap: [String: Float] = [
                "polyblep-saw": 2, "polyblep-square": 3, "polyblep-tri": 4,
                "noise-white": 5, "noise-pink": 5,
                "wt-analog": 6, "wt-vocal": 6,
                "fm-basic": 7 // Pulse (closest to FM in OBRIX)
            ]
            let engineType = srcTypeMap[specimen.subtype] ?? 2 // Default saw
            bridge.setParameterImmediate("obrix_src1Type", value: engineType)

        case .processor:
            // Map specimen processor type to engine proc type
            let procTypeMap: [String: Float] = [
                "svf-lp": 1, "svf-hp": 2, "svf-bp": 3,
                "shaper-soft": 4, "shaper-hard": 4,
                "feedback": 5
            ]
            let engineType = procTypeMap[specimen.subtype] ?? 1 // Default LP
            bridge.setParameterImmediate("obrix_proc1Type", value: engineType)

        case .modulator:
            // Enable mod slot 2 as LFO
            bridge.setParameterImmediate("obrix_mod2Type", value: 2) // LFO
            bridge.setParameterImmediate("obrix_mod2Target", value: 2) // Filter Cutoff

        case .effect:
            // Map specimen effect type to engine FX type
            let fxTypeMap: [String: Float] = [
                "delay-stereo": 1, "chorus-lush": 2,
                "reverb-hall": 3, "dist-warm": 1
            ]
            let engineType = fxTypeMap[specimen.subtype] ?? 1
            bridge.setParameterImmediate("obrix_fx1Type", value: engineType)
        }

        // Then: apply all mapped parameter values — continuous params are widened by rarity.
        // Build a dict first so bridge modifiers can overlay cleanly.
        let rMult = Self.rarityMultiplier(specimen.rarity)
        var paramDict: [String: Float] = [:]
        for (specimenParam, value) in specimen.parameterState {
            if let mapping = Self.parameterMapping[specimenParam] {
                let scaled = mapping.scale(value)
                paramDict[mapping.engineParam] = mapping.isContinuous ? (scaled * rMult) : scaled
            }
        }

        // Apply bridge modifiers (genetics + aging + memory + nursery) as additive deltas.
        // See collectMappedParams for full merge-strategy rationale.
        if let gc = gameCoordinator {
            let lifeStage = specimenAgingManager?.stage(for: slotIndex(of: specimen)) ?? .adult
            let playAge   = SpecimenAge.from(playSeconds: specimen.totalPlaySeconds)
            let bridgeMods = gc.effectiveAudioParameters(
                specimenId: specimen.id,
                subtypeId:  specimen.subtype,
                lifeStage:  lifeStage,
                playAge:    playAge
            )
            for (key, delta) in bridgeMods {
                paramDict[key] = (paramDict[key] ?? 0) + delta
            }
        }

        // Push all resolved params to the engine
        for (engineParam, finalValue) in paramDict {
            bridge.setParameterImmediate(engineParam, value: finalValue)
        }
    }

    /// Push a single specimen parameter change to the engine immediately.
    /// Used by the parameter panel sliders for real-time feedback.
    func pushSingleParam(specimenParam: String, value: Float) {
        guard let bridge = ObrixBridge.shared() else { return }
        if let mapping = Self.parameterMapping[specimenParam] {
            let scaled = mapping.scale(value)
            bridge.setParameterImmediate(mapping.engineParam, value: scaled)
        }
    }

    // MARK: - Specimen → Engine Parameter Mapping

    private struct ParamMapping {
        let engineParam: String
        let scale: (Float) -> Float
        var isContinuous: Bool = true  // false for discrete type selectors
    }

    // MARK: - Rarity Multipliers

    /// Widens continuous parameter ranges so rarer specimens sound more extreme.
    private static func rarityMultiplier(_ rarity: SpecimenRarity) -> Float {
        switch rarity {
        case .common:    return 1.0   // Standard ranges
        case .uncommon:  return 1.15  // 15% wider — slightly more interesting
        case .rare:      return 1.35  // 35% wider — noticeably different
        case .legendary: return 1.6   // 60% wider — sounds alien
        }
    }

    /// XP multiplier — rarer specimens level up faster to reward the harder catch.
    static func xpMultiplier(_ rarity: SpecimenRarity) -> Float {
        switch rarity {
        case .common:    return 1.0
        case .uncommon:  return 1.25
        case .rare:      return 1.5
        case .legendary: return 2.0
        }
    }

    // MARK: - Parameter Cache (T1-03)

    /// Per-slot cache of resolved (engineParam, scaledValue) pairs.
    /// Built once on wiring change; blasted atomically on each noteOn.
    private var slotParamCache: [Int: [(String, Float)]] = [:]

    /// Default values written before each cached blast to ensure modules not
    /// used by the current slot are reset cleanly.
    private static let resetParams: [(String, Float)] = [
        ("obrix_src1Type", 0), ("obrix_src2Type", 0),
        ("obrix_proc1Type", 0), ("obrix_proc2Type", 0), ("obrix_proc3Type", 0),
        ("obrix_fx1Type", 0), ("obrix_fx2Type", 0), ("obrix_fx3Type", 0),
        ("obrix_mod1Depth", 0), ("obrix_mod2Depth", 0),
        ("obrix_ampAttack", 0.01), ("obrix_ampDecay", 0.3),
        ("obrix_ampSustain", 0.7), ("obrix_ampRelease", 0.5),
    ]

    // MARK: - Specimen → Engine Parameter Mapping

    /// Maps specimen parameter names to OBRIX engine parameter names with value scaling.
    /// Specimen params use simplified names (flt1, env1, lfo1) and 0-1 ranges.
    /// Engine params use full OBRIX names (proc1, amp, mod2) and actual ranges.
    private static let parameterMapping: [String: ParamMapping] = [
        // Sources — type index needs offset (specimen 0=saw → engine 2=saw)
        "obrix_src1Type": ParamMapping(engineParam: "obrix_src1Type",
            scale: { v in
                // Specimen: 0=saw,1=square,2=tri,3=noise,4=wt,5=fm
                // Engine:   0=off,1=sine,2=saw,3=square,4=tri,5=noise,6=wt,7=pulse,8=driftwood
                let map: [Int: Float] = [0: 2, 1: 3, 2: 4, 3: 5, 4: 6, 5: 7]
                return map[Int(v)] ?? 2
            }, isContinuous: false),
        "obrix_src1Tune": ParamMapping(engineParam: "obrix_src1Tune",
            scale: { $0 }), // Both -24 to 24, direct
        "obrix_src1Level": ParamMapping(engineParam: "obrix_srcMix",
            scale: { $0 }), // 0-1 → 0-1

        // Processors — name and range remapping
        "obrix_flt1Cutoff": ParamMapping(engineParam: "obrix_proc1Cutoff",
            scale: { v in 20.0 + v * (20000.0 - 20.0) }), // 0-1 → 20-20000 Hz
        "obrix_flt1Resonance": ParamMapping(engineParam: "obrix_proc1Reso",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_flt1EnvDepth": ParamMapping(engineParam: "obrix_mod1Depth",
            scale: { $0 }), // -0.5 to 0.5 → -1 to 1 (close enough for now)

        // Envelope — name remapping + range scaling
        "obrix_env1Attack": ParamMapping(engineParam: "obrix_ampAttack",
            scale: { $0 }), // 0-0.3 → 0-10 (within range, just small values)
        "obrix_env1Decay": ParamMapping(engineParam: "obrix_ampDecay",
            scale: { $0 }), // 0-0.8 → 0-10
        "obrix_env1Sustain": ParamMapping(engineParam: "obrix_ampSustain",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_env1Release": ParamMapping(engineParam: "obrix_ampRelease",
            scale: { $0 }), // 0-2 → 0-20

        // Modulators — map to mod slot 2 (LFO)
        "obrix_lfo1Rate": ParamMapping(engineParam: "obrix_mod2Rate",
            scale: { $0 }), // 0.01-10 → 0.01-30 (within range)
        "obrix_lfo1Depth": ParamMapping(engineParam: "obrix_mod2Depth",
            scale: { $0 }), // 0-1 → -1 to 1 (positive only for now)
        "obrix_lfo1Shape": ParamMapping(engineParam: "obrix_mod2Type",
            scale: { v in
                // Specimen: 0=env,1=lfo,2=s&h,3=random → Engine: 0=off,1=env,2=lfo,3=vel,4=at
                let map: [Int: Float] = [0: 1, 1: 2, 2: 2, 3: 2]
                return map[Int(v)] ?? 2
            }, isContinuous: false),

        // Effects — map to fx slot 1
        "obrix_fx1Mix": ParamMapping(engineParam: "obrix_fx1Mix",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_fx1Param1": ParamMapping(engineParam: "obrix_fx1Param",
            scale: { $0 }), // 0-1 → 0-1
        "obrix_fx1Param2": ParamMapping(engineParam: "obrix_fx1Type",
            scale: { v in
                // Map continuous value to discrete FX type
                // 0=off, 1=delay, 2=chorus, 3=reverb
                if v < 0.25 { return 1 } // delay
                if v < 0.5  { return 2 } // chorus
                return 3 // reverb
            }, isContinuous: false),
    ]

    // MARK: - Note Input (from ReefGrid touch events)

    /// Reference to the reef store — set by ObrixPocketApp after launch
    weak var reefStoreRef: ReefStore?

    /// The slot currently selected as the "active" source for keyboard input.
    /// Set by ReefTab when the user taps a slot; keyboard noteOn/noteOff credit XP here.
    var activeSlot: Int?

    // MARK: - XP Tracking

    /// Records when each slot's note started (for duration-based XP on noteOff)
    private var noteOnTimestamps: [Int: Date] = [:]

    /// Award XP from external sources (Dive rewards, daily energy).
    /// Must be called on the main thread. Does a single atomic read-mutate-write.
    func awardBulkXP(slotIndex: Int, amount: Int) {
        assert(Thread.isMainThread, "awardBulkXP must be called on main thread")
        guard let reefStore = reefStoreRef,
              var specimen = reefStore.specimens[slotIndex] else { return }

        let mult = Self.xpMultiplier(specimen.rarity)
        specimen.xp += Int((Float(amount) * mult).rounded())

        checkAndApplyLevelUp(&specimen)

        reefStore.specimens[slotIndex] = specimen
    }

    /// Award XP to the specimen in the given slot, checking for level-up.
    /// Rarer specimens receive a multiplier so they progress faster (reward for harder catch).
    @available(*, deprecated, renamed: "awardBulkXP(slotIndex:amount:)",
               message: "Use awardBulkXP for external callers (Dive, daily energy). earnXP is called internally by noteOn/noteOff only.")
    func earnXP(slotIndex: Int, amount: Int) {
        guard let reefStore = reefStoreRef,
              var specimen = reefStore.specimens[slotIndex] else { return }
        let multiplied = Int((Float(amount) * Self.xpMultiplier(specimen.rarity)).rounded())
        specimen.xp += multiplied
        let newLevel = SpecimenLeveling.checkLevelUp(xp: specimen.xp)
        if newLevel > specimen.level {
            let oldLevel = specimen.level
            specimen.level = newLevel
            HapticEngine.levelUp()

            // Journal: level-up event (write into live specimen before assignment)
            let levelEntry = JournalEntry(id: UUID(), timestamp: Date(), type: .levelUp,
                                          description: "Reached level \(newLevel)")
            specimen.journal.append(levelEntry)

            // EVOLUTION: reaching level 10 for the first time triggers a name/identity transformation.
            // oldLevel guards against re-triggering if earnXP is called again after already hitting 10.
            if newLevel >= 10 && oldLevel < 10 {
                if let evolved = EvolutionCatalog.evolvedForm(for: specimen) {
                    specimen.name = evolved.name
                    // Keep original subtype — DSP stays the same; the evolved name is the reward
                    HapticEngine.evolution()

                    // Journal: evolution event
                    let evolveEntry = JournalEntry(id: UUID(), timestamp: Date(), type: .evolved,
                                                   description: "Evolved into \(evolved.name)")
                    specimen.journal.append(evolveEntry)
                }
            }
        }
        reefStore.specimens[slotIndex] = specimen
    }

    // MARK: - Level-Up / Evolution

    /// Check whether `spec` has accumulated enough XP to level up; if so, apply the level
    /// increment, haptics, stat tracking, journal entry, and evolution transform in one place.
    /// Call this after any XP mutation, before writing the specimen back to the store.
    /// Mutates `spec` in place — no store write performed here.
    private func checkAndApplyLevelUp(_ spec: inout Specimen) {
        let newLevel = SpecimenLeveling.checkLevelUp(xp: spec.xp)
        guard newLevel > spec.level else { return }

        let oldLevel = spec.level
        spec.level = newLevel
        HapticEngine.levelUp()
        ReefStatsTracker.shared.increment(.levelUps)
        let levelEntry = JournalEntry(id: UUID(), timestamp: Date(), type: .levelUp,
                                      description: "Reached level \(newLevel)")
        spec.journal.append(levelEntry)

        // EVOLUTION: reaching level 10 for the first time triggers a name/identity
        // transformation. oldLevel guards against re-triggering on subsequent XP awards.
        if newLevel >= 10 && oldLevel < 10 {
            if let evolved = EvolutionCatalog.evolvedForm(for: spec) {
                spec.name = evolved.name
                // Keep original subtype — DSP stays the same; the evolved name is the reward.
                HapticEngine.evolution()
                ReefStatsTracker.shared.increment(.evolutions)
                let evolveEntry = JournalEntry(id: UUID(), timestamp: Date(), type: .evolved,
                                               description: "Evolved into \(evolved.name)")
                spec.journal.append(evolveEntry)
            }
        }
    }

    func noteOn(slotIndex: Int, velocity: Float) {
        assert(Thread.isMainThread, "noteOn must be called on main thread")
        guard let bridge = ObrixBridge.shared() else { return }

        // Fast path: blast cached params (no UUID matching, no chain walking per note).
        // Falls back to full applySlotChain if cache is cold (first note before wiring settles).
        // C-01: No cache-miss fallback — rebuilding param cache from noteOn is a data race
        // when noteOn is called from a non-main thread (reads @Published reefStore.specimens).
        // rebuildParamCache is already called on every wiring change via onWiringChanged /
        // applyReefConfiguration. If the cache is cold, the engine keeps its last config.
        if let cached = slotParamCache[slotIndex] {
            for (param, value) in Self.resetParams {
                bridge.setParameterImmediate(param, value: value)
            }
            for (param, value) in cached {
                bridge.setParameterImmediate(param, value: value)
            }
        }
        // Cache miss: skip param blast — engine retains whatever was last configured.

        // Map slot index to MIDI note: C3 + slot index (gives C3-D#4 for 16 slots)
        let note = min(127, max(0, 60 + slotIndex))
        bridge.note(on: Int32(note), velocity: velocity)
        midiOutput.sendNoteOn(note: UInt8(note), velocity: UInt8(velocity * 127), channel: UInt8(slotIndex % 16))
        if !hasPlayedFirstNote {
            hasPlayedFirstNote = true
        }

        // XP: record note-on timestamp and award 1 XP per note.
        // C-02: Consolidate all per-note mutations into a single read-mutate-write so that
        // earnXP's write and the style-score write don't clobber each other.
        noteOnTimestamps[slotIndex] = Date()
        if let reefStore = reefStoreRef,
           var specimen = reefStore.specimens[slotIndex] {
            // XP with rarity multiplier
            let xpGain = Int((Float(1) * Self.xpMultiplier(specimen.rarity)).rounded())
            specimen.xp += xpGain

            // Style score
            if velocity > 0.7 {
                specimen.aggressiveScore += 0.1
            } else if velocity < 0.4 {
                specimen.gentleScore += 0.1
            }

            // Level-up check
            checkAndApplyLevelUp(&specimen)

            // Single write — no clobbering
            reefStore.specimens[slotIndex] = specimen
        }
    }

    func noteOff(slotIndex: Int) {
        assert(Thread.isMainThread, "noteOff must be called on main thread")
        // Clamp to valid MIDI note range 0-127
        let note = min(127, max(0, 60 + slotIndex))
        ObrixBridge.shared()?.noteOff(Int32(note))
        midiOutput.sendNoteOff(note: UInt8(note), channel: UInt8(slotIndex % 16))

        // XP: compute note duration, award bonuses, update play stats.
        // C-03: Consolidate into a single read-mutate-write. The old code called earnXP (which
        // writes specimens[slotIndex]) then wrote a stale local copy on top, losing the XP update.
        if let startTime = noteOnTimestamps.removeValue(forKey: slotIndex),
           let reefStore = reefStoreRef,
           var specimen = reefStore.specimens[slotIndex] {

            let duration = Date().timeIntervalSince(startTime)
            specimen.totalPlaySeconds += duration
            ReefStatsTracker.shared.increment(.playSeconds, by: max(1, Int(duration)))

            if duration > 2.0 {
                // Sustained note reward — XP with rarity multiplier + style score
                let xpGain = Int((Float(3) * Self.xpMultiplier(specimen.rarity)).rounded())
                specimen.xp += xpGain
                specimen.gentleScore += 0.2
            } else if duration < 0.3 {
                // Short staccato tap
                specimen.aggressiveScore += 0.1
            }

            // Level-up check after duration XP
            checkAndApplyLevelUp(&specimen)

            // Single write — no stale-copy clobber
            reefStore.specimens[slotIndex] = specimen
        }
    }

    func allNotesOff() {
        ObrixBridge.shared()?.allNotesOff()
    }

    // MARK: - Audio Session

    private func configureAudioSession() {
        let session = AVAudioSession.sharedInstance()
        do {
            // Start with playback — switch to playAndRecord when mic is needed (catch)
            try session.setCategory(.playback)
            try session.setPreferredIOBufferDuration(0.005) // 5ms = 256 samples at 48kHz
            let preferredRate: Double = UserDefaults.standard.integer(forKey: "audio.sampleRate") == 1 ? 48000.0 : 44100.0
            try session.setPreferredSampleRate(preferredRate)
            // setActive(true) is deferred to start() — called just before JUCE bridge starts
            audioSessionConfigured = true

            // Interruption handling
            NotificationCenter.default.addObserver(
                self,
                selector: #selector(handleInterruption),
                name: AVAudioSession.interruptionNotification,
                object: session
            )

            // Secondary audio hint (for ducking reef ambient under other media)
            NotificationCenter.default.addObserver(
                self,
                selector: #selector(handleSecondaryAudioHint),
                name: AVAudioSession.silenceSecondaryAudioHintNotification,
                object: session
            )

            NotificationCenter.default.addObserver(
                self,
                selector: #selector(handleRouteChange),
                name: AVAudioSession.routeChangeNotification,
                object: session
            )
        } catch {
            print("[ObrixPocket] Audio session config failed: \(error)")
        }
    }

    @objc private func handleInterruption(_ notification: Notification) {
        guard let info = notification.userInfo,
              let typeValue = info[AVAudioSessionInterruptionTypeKey] as? UInt,
              let type = AVAudioSession.InterruptionType(rawValue: typeValue) else { return }

        switch type {
        case .began:
            stop()
        case .ended:
            // Always attempt resume — InterruptionOptionKey may be absent on iOS 14+
            let shouldResume: Bool
            if let optionsValue = info[AVAudioSessionInterruptionOptionKey] as? UInt {
                shouldResume = AVAudioSession.InterruptionOptions(rawValue: optionsValue).contains(.shouldResume)
            } else {
                shouldResume = true  // Absent key = assume resume
            }
            if shouldResume {
                do {
                    try AVAudioSession.sharedInstance().setActive(true)
                } catch {
                    print("[ObrixPocket] Failed to reactivate audio session: \(error)")
                }
                start()
            }
        @unknown default:
            break
        }
    }

    @objc private func handleRouteChange(_ notification: Notification) {
        guard let info = notification.userInfo,
              let reasonValue = info[AVAudioSessionRouteChangeReasonKey] as? UInt,
              let reason = AVAudioSession.RouteChangeReason(rawValue: reasonValue) else { return }

        if reason == .oldDeviceUnavailable {
            // Headphones unplugged — pause briefly then resume on speaker
            stop()
            // Auto-resume on speaker after a brief delay
            DispatchQueue.main.asyncAfter(deadline: .now() + 0.5) { [weak self] in
                self?.start()
            }
        }
    }

    @objc private func handleSecondaryAudioHint(_ notification: Notification) {
        guard let info = notification.userInfo,
              let typeValue = info[AVAudioSessionSilenceSecondaryAudioHintTypeKey] as? UInt,
              let type = AVAudioSession.SilenceSecondaryAudioHintType(rawValue: typeValue) else { return }

        switch type {
        case .begin:
            // Other audio started — duck reef ambient to -12dB
            ObrixBridge.shared()?.setOutputGain(0.25) // -12dB
        case .end:
            // Other audio stopped — restore
            ObrixBridge.shared()?.setOutputGain(1.0)
        @unknown default:
            break
        }
    }

    deinit {
        NotificationCenter.default.removeObserver(self)
        // Ensure audio session is deactivated cleanly on teardown
        ObrixBridge.shared()?.stopAudio()
        try? AVAudioSession.sharedInstance().setActive(false, options: .notifyOthersOnDeactivation)
    }
}
