import SwiftUI

/// The Rosetta Stone — translates synth parameters into game-language stats.
/// Stats are always computed, never stored. Same params = same stats.
struct GameStats {
    let warmth: Int      // Cutoff frequency → 0-100 "How friendly?"
    let intensity: Int   // Resonance → 0-100 "How fierce?"
    let reflexes: Int    // Attack time (inverted) → 0-100 "How quick?"
    let stamina: Int     // Decay + Release → 0-100 "How long does it last?"
    let presence: Int    // Detune/Spread → 0-100 "How big?"
    let complexity: Int  // FM ratio/index → 0-100 "How weird/unique?"
    let pulse: Int       // LFO rate → 0-100 "How alive?"
    let grit: Int        // Drive/Saturation → 0-100 "How rough/smooth?"
    let aura: Int        // Wet/Dry (FX) → 0-100 "How much space?"

    /// Derive game stats from a specimen's parameter state
    static func from(params: [String: Float]) -> GameStats {
        GameStats(
            warmth:     Self.scale(params["obrix_flt1Cutoff"] ?? 0.5),
            intensity:  Self.scale(params["obrix_flt1Resonance"] ?? 0.2),
            reflexes:   Self.invert(params["obrix_env1Attack"] ?? 0.1, maxRaw: 0.5),
            stamina:    Self.scale(((params["obrix_env1Decay"] ?? 0.3) + (params["obrix_env1Release"] ?? 0.3)) / 2.0),
            presence:   Self.scale(params["obrix_src1Level"] ?? 0.7),
            complexity: Self.scale(params["obrix_src1Type"].map { $0 / 5.0 } ?? 0.3),
            pulse:      Self.scale(params["obrix_lfo1Rate"].map { min($0 / 10.0, 1.0) } ?? 0.3),
            grit:       Self.scale(params["obrix_fx1Param1"] ?? 0.3),
            aura:       Self.scale(params["obrix_fx1Mix"] ?? 0.3)
        )
    }

    /// Scale a 0-1 float to 0-100 int
    private static func scale(_ value: Float) -> Int {
        Int(max(0, min(100, value * 100)))
    }

    /// Invert and scale (for attack → reflexes: fast attack = high reflexes)
    private static func invert(_ value: Float, maxRaw: Float) -> Int {
        let normalized = max(0, min(1, value / maxRaw))
        return Int((1.0 - normalized) * 100)
    }

    /// Top 3 stats by value (for creature card summary)
    var topThree: [(String, Int)] {
        let all: [(String, Int)] = [
            ("Warmth", warmth), ("Intensity", intensity), ("Reflexes", reflexes),
            ("Stamina", stamina), ("Presence", presence), ("Complexity", complexity),
            ("Pulse", pulse), ("Grit", grit), ("Aura", aura)
        ]
        return Array(all.sorted { $0.1 > $1.1 }.prefix(3))
    }
}

// MARK: - Stat Bar View

/// A single game stat bar with label, value, and colored fill
struct StatBar: View {
    let label: String
    let value: Int
    let color: Color

    var body: some View {
        HStack(spacing: 8) {
            Text(label)
                .font(DesignTokens.body(10))
                .foregroundColor(.white.opacity(0.5))
                .frame(width: 70, alignment: .trailing)

            GeometryReader { geometry in
                ZStack(alignment: .leading) {
                    RoundedRectangle(cornerRadius: 2)
                        .fill(Color.white.opacity(0.06))
                        .frame(height: 6)

                    RoundedRectangle(cornerRadius: 2)
                        .fill(color.opacity(0.7))
                        .frame(width: geometry.size.width * CGFloat(value) / 100.0, height: 6)
                }
            }
            .frame(height: 6)

            Text("\(value)")
                .font(DesignTokens.mono(9))
                .foregroundColor(.white.opacity(0.4))
                .frame(width: 28, alignment: .trailing)
        }
        .frame(height: 14)
    }
}

// MARK: - Full Stats Panel

/// All 9 game stats rendered as bars
struct StatsPanel: View {
    let stats: GameStats
    let categoryColor: Color

    var body: some View {
        VStack(spacing: 4) {
            StatBar(label: "Warmth",     value: stats.warmth,     color: categoryColor)
            StatBar(label: "Intensity",  value: stats.intensity,  color: categoryColor)
            StatBar(label: "Reflexes",   value: stats.reflexes,   color: categoryColor)
            StatBar(label: "Stamina",    value: stats.stamina,    color: categoryColor)
            StatBar(label: "Presence",   value: stats.presence,   color: categoryColor)
            StatBar(label: "Complexity", value: stats.complexity, color: categoryColor)
            StatBar(label: "Pulse",      value: stats.pulse,      color: categoryColor)
            StatBar(label: "Grit",       value: stats.grit,       color: categoryColor)
            StatBar(label: "Aura",       value: stats.aura,       color: categoryColor)
        }
    }
}
