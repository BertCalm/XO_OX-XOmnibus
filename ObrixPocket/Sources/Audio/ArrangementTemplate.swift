import Foundation

// MARK: - Template Category

/// Broad classification of arrangement templates by complexity and intent.
enum TemplateCategory: String, CaseIterable, Codable {
    case starter   // 2-3 specimens, immediate gratification, teaches basic roles
    case ensemble  // 4-5 specimens, balanced arrangements, mid-complexity
    case orchestra // 6 specimens, full reef engaged, advanced play
    case genre     // Style-specific presets that teach genre aesthetics
}

// MARK: - Slot Definition

/// One slot in an arrangement template — the role requirement and hints for that position.
struct TemplateSlot: Codable {
    let index: Int
    let role: MusicalRole
    /// Suggested relative volume (0-1). The player can override.
    let volumeHint: Float
    /// Short hint shown to the player about what to put here.
    let hint: String
}

// MARK: - Coupling Suggestion

/// A suggested wiring between two template slots.
/// Both indices reference slot.index values in the same template.
struct CouplingHint: Codable {
    let fromSlot: Int
    let toSlot: Int
    let reason: String   // Why this coupling makes musical sense
}

// MARK: - ArrangementTemplate

/// A pre-built ensemble blueprint — teaches players how to combine specimens
/// and provides a framework for musical exploration.
struct ArrangementTemplate: Codable, Identifiable {

    // MARK: Properties
    let id: String
    let name: String
    let description: String
    let category: TemplateCategory

    /// Ordered slot definitions (length = requiredSlotCount)
    let slots: [TemplateSlot]

    /// Suggested BPM range for best results with this template
    let tempoRange: ClosedRange<Double>

    /// Scale that works best with this template
    let scalePreference: Scale

    /// Difficulty rating: 1 (trivial) to 5 (requires musical judgment)
    let difficultyLevel: Int

    /// Suggested wiring pairs between slots
    let couplingHints: [CouplingHint]

    /// 1-2 sentence performance tip shown to the player
    let performanceNote: String

    // MARK: Derived

    var requiredSlotCount: Int { slots.count }

    /// All roles required by this template (may contain duplicates)
    var requiredRoles: [MusicalRole] { slots.map { $0.role } }

    /// Unique set of roles this template needs at least one of
    var distinctRoles: Set<MusicalRole> { Set(requiredRoles) }
}

// MARK: - Slot Match Score

/// Result of evaluating one specimen against one slot.
struct SlotMatchScore {
    let specimenID: UUID
    let slotIndex: Int
    let score: Float   // 0 (worst) to 1 (perfect)
}

// MARK: - ArrangementTemplateManager

/// Manages the library of arrangement templates, autoassignment logic,
/// and player-saved custom templates.
final class ArrangementTemplateManager: ObservableObject {

    // MARK: - Published State

    @Published var activeTemplate: ArrangementTemplate?
    /// Maps slot index → specimen ID for the current active template
    @Published var assignments: [Int: UUID] = [:]
    @Published var customTemplates: [ArrangementTemplate] = []

    // MARK: - Persistence

    private let customTemplatesKey = "obrix_custom_templates"

    // MARK: - Init

    init() {
        loadCustomTemplates()
    }

    // MARK: - Template Library

    /// All 20 built-in templates, immutable.
    static let library: [ArrangementTemplate] = buildLibrary()

    // MARK: - Template Selection

    /// Activate a template, clearing any previous assignments.
    func activate(_ template: ArrangementTemplate) {
        activeTemplate = template
        assignments = [:]
    }

    /// Deactivate the current template.
    func deactivate() {
        activeTemplate = nil
        assignments = [:]
    }

    // MARK: - Specimen Matching

    /// Score how well a single specimen's VoiceProfile fits a given slot.
    /// Returns a value 0-1 (higher = better fit).
    func score(subtypeID: String, specimenID: UUID, against slot: TemplateSlot) -> SlotMatchScore {
        let profile = VoiceProfileCatalog.profile(for: subtypeID)
        let role = SpecimenRoleMap.role(for: subtypeID)
        var s: Float = 0

        // Primary: exact role match
        if role == slot.role { s += 0.6 }

        // Secondary: anchor compatibility
        if slot.role == .bass && profile.anchor { s += 0.2 }
        if slot.role == .melody && !profile.anchor { s += 0.1 }

        // Tertiary: attack sharpness suitability
        switch slot.role {
        case .rhythm:
            // Rhythm slots want sharp attacks
            s += profile.attackSharpness * 0.1
        case .texture, .effect:
            // Texture/effect slots want soft attacks
            s += (1.0 - profile.attackSharpness) * 0.1
        case .bass, .harmony:
            // Bass/harmony want moderate to long sustain
            s += profile.sustainLength * 0.1
        case .melody:
            // Melody benefits from velocity variation (expressiveness)
            s += profile.velocityVariation * 0.1
        }

        return SlotMatchScore(specimenID: specimenID, slotIndex: slot.index, score: min(1.0, s))
    }

    /// Score all specimens against all slots in a template and return a sorted
    /// list of (slotIndex → [SlotMatchScore]) with best matches first per slot.
    func matchSpecimensToTemplate(
        _ template: ArrangementTemplate,
        specimens: [(id: UUID, subtypeID: String)]
    ) -> [Int: [SlotMatchScore]] {
        var result: [Int: [SlotMatchScore]] = [:]

        for slot in template.slots {
            let scores = specimens.map { spec in
                score(subtypeID: spec.subtypeID, specimenID: spec.id, against: slot)
            }.sorted { $0.score > $1.score }
            result[slot.index] = scores
        }

        return result
    }

    /// Best-effort greedy assignment of specimens to slots.
    /// Each specimen is used at most once. Returns slot index → specimen ID.
    /// Call this to auto-populate `assignments`.
    @discardableResult
    func autoAssign(
        template: ArrangementTemplate,
        specimens: [(id: UUID, subtypeID: String)]
    ) -> [Int: UUID] {
        let matchMap = matchSpecimensToTemplate(template, specimens: specimens)

        var used: Set<UUID> = []
        var result: [Int: UUID] = [:]

        // Process slots in order
        for slot in template.slots.sorted(by: { $0.index < $1.index }) {
            guard let candidates = matchMap[slot.index] else { continue }
            // Pick the best-scoring specimen that hasn't been used yet
            if let best = candidates.first(where: { !used.contains($0.specimenID) }) {
                result[slot.index] = best.specimenID
                used.insert(best.specimenID)
            }
        }

        assignments = result
        activeTemplate = template
        return result
    }

    // MARK: - Assign / Unassign

    func assign(specimenID: UUID, toSlot slotIndex: Int) {
        assignments[slotIndex] = specimenID
    }

    func unassign(slotIndex: Int) {
        assignments.removeValue(forKey: slotIndex)
    }

    var isFullyAssigned: Bool {
        guard let template = activeTemplate else { return false }
        return template.slots.allSatisfy { assignments[$0.index] != nil }
    }

    // MARK: - Custom Templates

    func saveCustomTemplate(_ template: ArrangementTemplate) {
        customTemplates.append(template)
        persistCustomTemplates()
    }

    func deleteCustomTemplate(id: String) {
        customTemplates.removeAll { $0.id == id }
        persistCustomTemplates()
    }

    private func persistCustomTemplates() {
        if let data = try? JSONEncoder().encode(customTemplates) {
            UserDefaults.standard.set(data, forKey: customTemplatesKey)
        }
    }

    private func loadCustomTemplates() {
        guard let data = UserDefaults.standard.data(forKey: customTemplatesKey),
              let saved = try? JSONDecoder().decode([ArrangementTemplate].self, from: data) else { return }
        customTemplates = saved
    }

    // MARK: - All Templates (library + custom)

    var allTemplates: [ArrangementTemplate] {
        Self.library + customTemplates
    }

    var templatesByCategory: [TemplateCategory: [ArrangementTemplate]] {
        Dictionary(grouping: allTemplates, by: { $0.category })
    }
}

// MARK: - Library Builder

private func buildLibrary() -> [ArrangementTemplate] {
    var lib: [ArrangementTemplate] = []

    // ═══════════════════════════════════════
    // STARTER TEMPLATES (5)
    // ═══════════════════════════════════════

    lib.append(ArrangementTemplate(
        id: "starter-first-duet",
        name: "First Duet",
        description: "Two voices — one holds the bottom, one sings the top. The simplest musical conversation.",
        category: .starter,
        slots: [
            TemplateSlot(index: 0, role: .bass,   volumeHint: 0.75, hint: "Anchor — holds the root"),
            TemplateSlot(index: 1, role: .melody, volumeHint: 0.85, hint: "Singer — moves freely above"),
        ],
        tempoRange: 70...110,
        scalePreference: .pentatonic,
        difficultyLevel: 1,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass drives the melody's filter, making it bright when the bass hits hard")
        ],
        performanceNote: "Let the bass breathe first, then introduce the melody. The pentatonic scale means nothing will clash."
    ))

    lib.append(ArrangementTemplate(
        id: "starter-triad",
        name: "Triad",
        description: "Three voices covering the full harmonic picture: foundation, color, and movement.",
        category: .starter,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.70, hint: "Foundation — low and steady"),
            TemplateSlot(index: 1, role: .harmony, volumeHint: 0.60, hint: "Color — fills the middle"),
            TemplateSlot(index: 2, role: .melody,  volumeHint: 0.80, hint: "Movement — leads the phrase"),
        ],
        tempoRange: 75...115,
        scalePreference: .major,
        difficultyLevel: 1,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass to harmony: root note guides chord voicing"),
            CouplingHint(fromSlot: 1, toSlot: 2, reason: "Harmony to melody: chord tones suggest which notes to choose"),
        ],
        performanceNote: "This is the building block of almost all music. Once it feels natural, every other template will make sense."
    ))

    lib.append(ArrangementTemplate(
        id: "starter-pulse-and-pad",
        name: "Pulse & Pad",
        description: "A rhythmic engine under a cloud of sustained texture — motion and stillness together.",
        category: .starter,
        slots: [
            TemplateSlot(index: 0, role: .rhythm,  volumeHint: 0.70, hint: "Pulse — drives the energy"),
            TemplateSlot(index: 1, role: .texture, volumeHint: 0.50, hint: "Atmosphere — drifts behind everything"),
        ],
        tempoRange: 80...130,
        scalePreference: .dorian,
        difficultyLevel: 1,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Rhythm triggers texture modulation on every hit")
        ],
        performanceNote: "Turn the texture volume low and let the rhythm speak first. Then slowly raise the pad until they feel like one organism."
    ))

    lib.append(ArrangementTemplate(
        id: "starter-call-and-response",
        name: "Call & Response",
        description: "Two voices take turns. One asks, the other answers. Ancient conversation.",
        category: .starter,
        slots: [
            TemplateSlot(index: 0, role: .melody, volumeHint: 0.80, hint: "Caller — asks the question"),
            TemplateSlot(index: 1, role: .melody, volumeHint: 0.75, hint: "Responder — answers"),
        ],
        tempoRange: 65...100,
        scalePreference: .blues,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "First melody's phrase energy triggers response in second"),
        ],
        performanceNote: "Try two specimens with different attack characters — one sharp, one soft. The contrast makes the conversation vivid."
    ))

    lib.append(ArrangementTemplate(
        id: "starter-simple-loop",
        name: "Simple Loop",
        description: "Bass, rhythm, and a lead — the skeleton of a groove. Everything else is decoration.",
        category: .starter,
        slots: [
            TemplateSlot(index: 0, role: .bass,   volumeHint: 0.75, hint: "Root — the floor"),
            TemplateSlot(index: 1, role: .rhythm, volumeHint: 0.80, hint: "Groove — the pulse"),
            TemplateSlot(index: 2, role: .melody, volumeHint: 0.70, hint: "Lead — the hook"),
        ],
        tempoRange: 85...130,
        scalePreference: .minor,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass to rhythm: bass note timing tightens the drum hits"),
            CouplingHint(fromSlot: 1, toSlot: 2, reason: "Rhythm to melody: drum accents trigger melodic emphasis"),
        ],
        performanceNote: "This template works at any tempo. Start slow, find the groove, then push the BPM until it lifts off."
    ))

    // ═══════════════════════════════════════
    // ENSEMBLE TEMPLATES (6)
    // ═══════════════════════════════════════

    lib.append(ArrangementTemplate(
        id: "ensemble-jazz-quartet",
        name: "Jazz Quartet",
        description: "Walking bass, comping chords, a singing melody, and a steady rhythmic backbone.",
        category: .ensemble,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.70, hint: "Walk — stepwise bass lines"),
            TemplateSlot(index: 1, role: .harmony, volumeHint: 0.55, hint: "Comp — offbeat chord stabs"),
            TemplateSlot(index: 2, role: .melody,  volumeHint: 0.85, hint: "Horn — the improvised lead"),
            TemplateSlot(index: 3, role: .rhythm,  volumeHint: 0.65, hint: "Brushes — light rhythmic texture"),
        ],
        tempoRange: 90...160,
        scalePreference: .dorian,
        difficultyLevel: 3,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass motion informs chord voicing placement"),
            CouplingHint(fromSlot: 3, toSlot: 2, reason: "Rhythm accent guides melody emphasis"),
        ],
        performanceNote: "Swing is everything here. Make sure your rhythm specimen has swing enabled, and let the harmony comp behind the beat."
    ))

    lib.append(ArrangementTemplate(
        id: "ensemble-ambient-layers",
        name: "Ambient Layers",
        description: "Five slow textures stacked like water columns — each one at a different depth.",
        category: .ensemble,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.55, hint: "Depth — sub resonance"),
            TemplateSlot(index: 1, role: .texture, volumeHint: 0.45, hint: "Murk — mid drones"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.50, hint: "Shimmer — chord washes"),
            TemplateSlot(index: 3, role: .texture, volumeHint: 0.40, hint: "Air — high frequency hiss"),
            TemplateSlot(index: 4, role: .melody,  volumeHint: 0.55, hint: "Glint — occasional single notes"),
        ],
        tempoRange: 40...72,
        scalePreference: .pentatonic,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 2, reason: "Bass movement slowly modulates harmony filter cutoff"),
            CouplingHint(fromSlot: 2, toSlot: 4, reason: "Chord brightness influences which melody notes emerge"),
        ],
        performanceNote: "Set all volumes low and let it breathe. The beauty is in what isn't there as much as what is."
    ))

    lib.append(ArrangementTemplate(
        id: "ensemble-drum-circle",
        name: "Drum Circle",
        description: "Four rhythmic voices with interlocking patterns — no melody needed when the groove is this full.",
        category: .ensemble,
        slots: [
            TemplateSlot(index: 0, role: .bass,   volumeHint: 0.80, hint: "Kick — the downbeat anchor"),
            TemplateSlot(index: 1, role: .rhythm, volumeHint: 0.75, hint: "Cross — the syncopated pulse"),
            TemplateSlot(index: 2, role: .rhythm, volumeHint: 0.65, hint: "Fill — the off-beat ghost"),
            TemplateSlot(index: 3, role: .rhythm, volumeHint: 0.60, hint: "Color — the accent snap"),
        ],
        tempoRange: 90...140,
        scalePreference: .pentatonic,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Kick locks the cross-pattern timing"),
            CouplingHint(fromSlot: 1, toSlot: 2, reason: "Cross accent triggers fill cascade"),
        ],
        performanceNote: "Find specimens with very different attack characters. The best drum circles have contrast — hard next to soft, long next to short."
    ))

    lib.append(ArrangementTemplate(
        id: "ensemble-chamber-music",
        name: "Chamber Music",
        description: "Five voices of equal importance, each speaking and listening in turn.",
        category: .ensemble,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.65, hint: "Cello — low melodic anchor"),
            TemplateSlot(index: 1, role: .harmony, volumeHint: 0.60, hint: "Viola — inner voice"),
            TemplateSlot(index: 2, role: .melody,  volumeHint: 0.75, hint: "First violin — leads"),
            TemplateSlot(index: 3, role: .harmony, volumeHint: 0.55, hint: "Second violin — follows"),
            TemplateSlot(index: 4, role: .texture, volumeHint: 0.35, hint: "Room — the space between"),
        ],
        tempoRange: 60...120,
        scalePreference: .major,
        difficultyLevel: 3,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass feeds cello line to viola for harmonic awareness"),
            CouplingHint(fromSlot: 2, toSlot: 3, reason: "First violin phrase shapes second violin response"),
            CouplingHint(fromSlot: 4, toSlot: 2, reason: "Room texture subtly colors the lead melody"),
        ],
        performanceNote: "This arrangement rewards patience. Let each voice complete its phrase before changing anything."
    ))

    lib.append(ArrangementTemplate(
        id: "ensemble-electronic-trio",
        name: "Electronic Trio",
        description: "Synthesizer bass, chord stabs, and an arpeggiated lead — the foundation of electronic music.",
        category: .ensemble,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.80, hint: "Sub — the weight"),
            TemplateSlot(index: 1, role: .harmony, volumeHint: 0.65, hint: "Stabs — the chords"),
            TemplateSlot(index: 2, role: .melody,  volumeHint: 0.75, hint: "Arp — the sequence"),
            TemplateSlot(index: 3, role: .effect,  volumeHint: 0.40, hint: "Space — reverb and delay"),
        ],
        tempoRange: 110...145,
        scalePreference: .minor,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 2, reason: "Bass root note locks arpeggio to correct octave"),
            CouplingHint(fromSlot: 1, toSlot: 3, reason: "Chord hits trigger spatial effect emphasis"),
        ],
        performanceNote: "Keep the bass dry and punchy, the stabs short, and let the arp breathe through the effect."
    ))

    lib.append(ArrangementTemplate(
        id: "ensemble-world-fusion",
        name: "World Fusion",
        description: "Drone bass, melodic conversation, and an interlocking rhythmic weave from two time-feel sources.",
        category: .ensemble,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.60, hint: "Drone — the tonic pedal"),
            TemplateSlot(index: 1, role: .melody,  volumeHint: 0.80, hint: "Lead melody — ornamental"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.55, hint: "Call — the modal color"),
            TemplateSlot(index: 3, role: .rhythm,  volumeHint: 0.70, hint: "Downbeat — primary pulse"),
            TemplateSlot(index: 4, role: .rhythm,  volumeHint: 0.60, hint: "Upbeat — secondary interlocking"),
        ],
        tempoRange: 75...120,
        scalePreference: .phrygian,
        difficultyLevel: 3,
        couplingHints: [
            CouplingHint(fromSlot: 3, toSlot: 4, reason: "Downbeat and upbeat rhythms interlock for polyrhythmic feel"),
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Drone tonic guides melodic ornament selection"),
        ],
        performanceNote: "Phrygian mode gives this its distinctive Mediterranean-meets-ocean character. The two rhythm voices should feel like they're having an argument."
    ))

    // ═══════════════════════════════════════
    // ORCHESTRA TEMPLATES (4)
    // ═══════════════════════════════════════

    lib.append(ArrangementTemplate(
        id: "orchestra-full-reef",
        name: "Full Reef Orchestra",
        description: "Every part of the reef speaking at once — bass, harmony, rhythm, melody, texture, and space.",
        category: .orchestra,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.70, hint: "Foundation — the reef floor"),
            TemplateSlot(index: 1, role: .harmony, volumeHint: 0.60, hint: "Chord mass — mid-water"),
            TemplateSlot(index: 2, role: .melody,  volumeHint: 0.80, hint: "Lead — breaks the surface"),
            TemplateSlot(index: 3, role: .rhythm,  volumeHint: 0.65, hint: "Current — the pulse"),
            TemplateSlot(index: 4, role: .texture, volumeHint: 0.40, hint: "Shimmer — high color"),
            TemplateSlot(index: 5, role: .effect,  volumeHint: 0.35, hint: "Space — the ocean itself"),
        ],
        tempoRange: 80...120,
        scalePreference: .major,
        difficultyLevel: 4,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass anchors chord changes"),
            CouplingHint(fromSlot: 3, toSlot: 2, reason: "Rhythm accents drive melody forward"),
            CouplingHint(fromSlot: 1, toSlot: 4, reason: "Harmony brightness controls texture shimmer rate"),
            CouplingHint(fromSlot: 2, toSlot: 5, reason: "Melody amplitude feeds spatial reverb depth"),
        ],
        performanceNote: "Start with only bass and rhythm. Add harmony. Then melody. Then texture and effect last — let each layer settle before adding the next."
    ))

    lib.append(ArrangementTemplate(
        id: "orchestra-symphonic-sweep",
        name: "Symphonic Sweep",
        description: "Six voices building from silence to full orchestral density. The arrangement structure does the heavy lifting.",
        category: .orchestra,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.65, hint: "Low strings — slow bow"),
            TemplateSlot(index: 1, role: .bass,    volumeHint: 0.55, hint: "Deep brass — pedal tone"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.60, hint: "Strings — the body"),
            TemplateSlot(index: 3, role: .melody,  volumeHint: 0.85, hint: "Winds — the theme"),
            TemplateSlot(index: 4, role: .texture, volumeHint: 0.45, hint: "Strings trem — the agitation"),
            TemplateSlot(index: 5, role: .rhythm,  volumeHint: 0.50, hint: "Timpani — punctuation"),
        ],
        tempoRange: 60...100,
        scalePreference: .minor,
        difficultyLevel: 4,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 2, reason: "Low bass drives string swell intensity"),
            CouplingHint(fromSlot: 5, toSlot: 3, reason: "Timpani hit triggers brass accent"),
            CouplingHint(fromSlot: 4, toSlot: 3, hint: "Tremolo tension modulates melodic urgency"),
        ],
        performanceNote: "The two bass voices together create a much larger low end than one alone. Balance them so neither overpowers."
    ))

    lib.append(ArrangementTemplate(
        id: "orchestra-deep-ensemble",
        name: "Deep Ensemble",
        description: "Six voices designed for the midnight and abyssal zones — dark, vast, and unhurried.",
        category: .orchestra,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.55, hint: "Sub drone — the pressure"),
            TemplateSlot(index: 1, role: .texture, volumeHint: 0.45, hint: "Depth noise — formless dark"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.50, hint: "Slow chord — ancient harmony"),
            TemplateSlot(index: 3, role: .texture, volumeHint: 0.35, hint: "Shimmer — distant bioluminescence"),
            TemplateSlot(index: 4, role: .melody,  volumeHint: 0.55, hint: "Signal — rare, high, haunted"),
            TemplateSlot(index: 5, role: .effect,  volumeHint: 0.40, hint: "Abyss — the space that swallows"),
        ],
        tempoRange: 35...65,
        scalePreference: .phrygian,
        difficultyLevel: 4,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass pressure builds texture swell"),
            CouplingHint(fromSlot: 2, toSlot: 3, reason: "Chord change triggers shimmer burst"),
            CouplingHint(fromSlot: 4, toSlot: 5, reason: "Melodic signal decays into the spatial effect"),
        ],
        performanceNote: "Use specimens with high sustain and low attack sharpness. In the deep, everything is slow. Resist the urge to speed up."
    ))

    lib.append(ArrangementTemplate(
        id: "orchestra-complete-reef",
        name: "The Complete Reef",
        description: "Every role, every layer, maximum complexity. Only for reefs with deep collections.",
        category: .orchestra,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.70, hint: "Root — the non-negotiable floor"),
            TemplateSlot(index: 1, role: .bass,    volumeHint: 0.50, hint: "Second bass — harmonic depth"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.55, hint: "Pads — the body"),
            TemplateSlot(index: 3, role: .melody,  volumeHint: 0.80, hint: "Primary lead — the voice"),
            TemplateSlot(index: 4, role: .rhythm,  volumeHint: 0.65, hint: "Main pulse — drives everything"),
            TemplateSlot(index: 5, role: .effect,  volumeHint: 0.30, hint: "Space — glues it all together"),
        ],
        tempoRange: 90...125,
        scalePreference: .mixolydian,
        difficultyLevel: 5,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 2, reason: "Bass root defines pad voicing"),
            CouplingHint(fromSlot: 4, toSlot: 3, reason: "Rhythm accent emphasizes melodic peaks"),
            CouplingHint(fromSlot: 2, toSlot: 5, reason: "Pad dynamics breathe with spatial effect depth"),
            CouplingHint(fromSlot: 1, toSlot: 0, reason: "Second bass enriches primary root with color tones"),
        ],
        performanceNote: "With six slots full, every specimen matters equally. Listen for the one that dominates — usually it needs a volume reduction, not elimination."
    ))

    // ═══════════════════════════════════════
    // GENRE TEMPLATES (5)
    // ═══════════════════════════════════════

    lib.append(ArrangementTemplate(
        id: "genre-lo-fi-reef",
        name: "Lo-Fi Reef",
        description: "Dusty keys, a warm bass, and a shuffling groove. The sound of studying underwater.",
        category: .genre,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.65, hint: "Upright bass — wobbly and warm"),
            TemplateSlot(index: 1, role: .harmony, volumeHint: 0.55, hint: "Keys — jazzy chord voicings"),
            TemplateSlot(index: 2, role: .rhythm,  volumeHint: 0.60, hint: "Drums — shuffled and loose"),
            TemplateSlot(index: 3, role: .texture, volumeHint: 0.30, hint: "Vinyl — the warm noise underneath"),
        ],
        tempoRange: 65...90,
        scalePreference: .dorian,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 2, toSlot: 3, reason: "Drum swing timing bleeds into vinyl saturation"),
            CouplingHint(fromSlot: 0, toSlot: 1, reason: "Bass motion telegraphs chord changes"),
        ],
        performanceNote: "High swing, low velocity variation, slow attack. Everything should feel like it's slightly behind the beat — that's the lo-fi pocket."
    ))

    lib.append(ArrangementTemplate(
        id: "genre-boom-bap-beach",
        name: "Boom Bap Beach",
        description: "Hard-hitting drums, a thick bass, and a chopped melody on top. Hip-hop washed ashore.",
        category: .genre,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.80, hint: "Sub bass — the boom"),
            TemplateSlot(index: 1, role: .rhythm,  volumeHint: 0.85, hint: "Kick/snare — the bap"),
            TemplateSlot(index: 2, role: .melody,  volumeHint: 0.65, hint: "Sample chop — the hook"),
            TemplateSlot(index: 3, role: .harmony, volumeHint: 0.45, hint: "Stab — the punctuation"),
        ],
        tempoRange: 85...100,
        scalePreference: .minor,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 1, toSlot: 0, reason: "Kick hit sidechain-shapes bass amplitude"),
            CouplingHint(fromSlot: 0, toSlot: 2, reason: "Bass root anchors melody chop pitch"),
        ],
        performanceNote: "90-95 BPM is the classic tempo. The kick needs maximum attack sharpness and minimum sustain. Make it punch."
    ))

    lib.append(ArrangementTemplate(
        id: "genre-techno-tide",
        name: "Techno Tide",
        description: "Four-on-the-floor kick, acid bass, and a relentless filter sweep. Industrial ocean.",
        category: .genre,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.80, hint: "Acid bass — the 303 line"),
            TemplateSlot(index: 1, role: .rhythm,  volumeHint: 0.90, hint: "Kick — four on the floor"),
            TemplateSlot(index: 2, role: .texture, volumeHint: 0.55, hint: "Filter sweep — the arc"),
            TemplateSlot(index: 3, role: .effect,  volumeHint: 0.40, hint: "Delay — the echo chamber"),
            TemplateSlot(index: 4, role: .harmony, volumeHint: 0.35, hint: "Stab — the occasional release"),
        ],
        tempoRange: 128...145,
        scalePreference: .minor,
        difficultyLevel: 3,
        couplingHints: [
            CouplingHint(fromSlot: 1, toSlot: 2, reason: "Kick energy drives filter sweep accumulation"),
            CouplingHint(fromSlot: 0, toSlot: 3, reason: "Acid line pitch changes extend into delay feedback"),
        ],
        performanceNote: "This template is about relentlessness. Don't change anything for the first minute. Let the repetition build the tension."
    ))

    lib.append(ArrangementTemplate(
        id: "genre-ambient-ocean",
        name: "Ambient Ocean",
        description: "Time slows. Notes ring for seconds. Changes happen in geological time. You're not listening — you're submerged.",
        category: .genre,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.45, hint: "Deep tonic — barely audible"),
            TemplateSlot(index: 1, role: .texture, volumeHint: 0.50, hint: "Wash — the ocean floor"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.40, hint: "Slow chord — changes every 8 bars"),
            TemplateSlot(index: 3, role: .effect,  volumeHint: 0.55, hint: "Hall — reverb as environment"),
            TemplateSlot(index: 4, role: .melody,  volumeHint: 0.45, hint: "Signal — very occasional, quiet"),
        ],
        tempoRange: 40...60,
        scalePreference: .wholeTone,
        difficultyLevel: 2,
        couplingHints: [
            CouplingHint(fromSlot: 0, toSlot: 2, reason: "Bass tonic subtly guides harmony root"),
            CouplingHint(fromSlot: 3, toSlot: 4, reason: "Reverb tail feeds back into melody sustain"),
        ],
        performanceNote: "Set your tempo to 50 BPM. Everything should have long sustain and near-zero attack sharpness. Whole tone scale means no dissonance — ever."
    ))

    lib.append(ArrangementTemplate(
        id: "genre-funk-coral",
        name: "Funk Coral",
        description: "Slap bass, tight snare, clavi stabs, and a wah lead. The reef gets funky.",
        category: .genre,
        slots: [
            TemplateSlot(index: 0, role: .bass,    volumeHint: 0.80, hint: "Slap — percussive, tight"),
            TemplateSlot(index: 1, role: .rhythm,  volumeHint: 0.75, hint: "Snare — crisp and on the 2-4"),
            TemplateSlot(index: 2, role: .harmony, volumeHint: 0.65, hint: "Clavi stabs — muted and rhythmic"),
            TemplateSlot(index: 3, role: .melody,  volumeHint: 0.70, hint: "Wah lead — expressive and greasy"),
            TemplateSlot(index: 4, role: .texture, volumeHint: 0.40, hint: "Horns stab — the punctuation"),
        ],
        tempoRange: 95...120,
        scalePreference: .mixolydian,
        difficultyLevel: 3,
        couplingHints: [
            CouplingHint(fromSlot: 1, toSlot: 0, reason: "Snare backbeat tightens bass mute timing"),
            CouplingHint(fromSlot: 2, toSlot: 3, reason: "Clavi stab rhythm suggests melodic entry points"),
            CouplingHint(fromSlot: 3, toSlot: 4, reason: "Wah lead intensity triggers horn punctuation"),
        ],
        performanceNote: "Funk lives in the pockets between the beats. High velocity variation, strong accent beats, and tight sustain on everything except the lead."
    ))

    return lib
}

// MARK: - CouplingHint Codable (extra init for hint vs reason ambiguity)

extension CouplingHint {
    init(fromSlot: Int, toSlot: Int, hint: String) {
        self.init(fromSlot: fromSlot, toSlot: toSlot, reason: hint)
    }
}
