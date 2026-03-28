import SwiftUI
import MediaPlayer

/// The #WhatsYourSong flow: pick a song → hash → chest ceremony → creature card → add to reef.
/// This is the Monster Rancher DNA moment — deterministic specimen from music metadata.
struct MusicCatchFlow: View {
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager
    @Environment(\.dismiss) private var dismiss

    @State private var phase: MusicCatchPhase = .picker
    @State private var selectedSong: SongMetadata?
    @State private var generatedSpecimen: Specimen?
    @State private var hashSeed: SpecimenHashGenerator.SpecimenSeed?
    @State private var hashAnimationProgress: Float = 0
    @State private var showReefFullAlert = false

    var body: some View {
        ZStack {
            DesignTokens.darkBackground.ignoresSafeArea()

            switch phase {
            case .picker:
                pickerPhase
            case .hashing:
                hashAnimationPhase
            case .ceremony:
                ceremonyPhase
            case .card:
                cardPhase
            }
        }
        .alert("Music Library Access", isPresented: $showDeniedAlert) {
            Button("Open Settings") {
                if let url = URL(string: UIApplication.openSettingsURLString) {
                    UIApplication.shared.open(url)
                }
            }
            Button("Cancel", role: .cancel) {}
        } message: {
            Text("OBRIX Pocket needs access to your music library to create creatures from songs. Enable it in Settings.")
        }
    }

    // MARK: - Phase 1: Song Picker

    private var pickerPhase: some View {
        Group {
            if firstLaunchManager.canMusicCatch {
                availablePickerView
            } else {
                cooldownView
            }
        }
        .sheet(isPresented: $showMusicPicker) {
            MusicPicker(
                onSongSelected: { artist, title, album, duration in
                    selectedSong = SongMetadata(artist: artist, title: title, album: album, duration: duration)
                    showMusicPicker = false
                    beginHashAnimation()
                },
                onCancel: {
                    showMusicPicker = false
                }
            )
        }
        .alert("Reef Full", isPresented: $showReefFullAlert) {
            Button("OK") { dismiss() }
        } message: {
            Text("Your reef has no empty slots. Release a specimen to make room.")
        }
    }

    private var availablePickerView: some View {
        VStack(spacing: 24) {
            Spacer()

            // Header
            VStack(spacing: 8) {
                Text("WHAT'S YOUR SONG?")
                    .font(.custom("SpaceGrotesk-Bold", size: 22))
                    .tracking(2)
                    .foregroundColor(.white)
                Text("Feed a song to the reef.\nEvery song births a unique creature.")
                    .font(.custom("Inter-Regular", size: 14))
                    .foregroundColor(.white.opacity(0.5))
                    .multilineTextAlignment(.center)
            }

            // Animated DNA helix icon
            Image(systemName: "waveform.path.ecg")
                .font(.system(size: 64))
                .foregroundColor(DesignTokens.reefJade.opacity(0.4))
                .padding(.vertical, 40)

            // Open Library button
            Button(action: { openMusicPicker() }) {
                Text("CHOOSE A SONG")
                    .font(.custom("SpaceGrotesk-Bold", size: 16))
                    .tracking(2)
                    .foregroundColor(.white)
                    .frame(maxWidth: .infinity)
                    .frame(height: 56)
                    .background(
                        RoundedRectangle(cornerRadius: 28)
                            .fill(DesignTokens.reefJade)
                    )
            }
            .padding(.horizontal, 40)

            Button("Cancel") { dismiss() }
                .font(.custom("Inter-Regular", size: 14))
                .foregroundColor(.white.opacity(0.4))

            Spacer()
        }
    }

    private var cooldownView: some View {
        VStack(spacing: 24) {
            Spacer()

            // Icon — dimmed
            Image(systemName: "moon.zzz")
                .font(.system(size: 64))
                .foregroundColor(DesignTokens.reefJade.opacity(0.25))
                .padding(.vertical, 20)

            VStack(spacing: 10) {
                Text("SONG OF THE DAY CAUGHT")
                    .font(.custom("SpaceGrotesk-Bold", size: 18))
                    .tracking(2)
                    .foregroundColor(.white.opacity(0.7))

                Text("You've already caught your Song of the Day.\nCome back tomorrow.")
                    .font(.custom("Inter-Regular", size: 14))
                    .foregroundColor(.white.opacity(0.4))
                    .multilineTextAlignment(.center)
                    .padding(.horizontal, 32)

                if let next = firstLaunchManager.nextMusicCatchDate {
                    let formatted = next.formatted(date: .omitted, time: .shortened)
                    Text("Next catch: tomorrow at \(formatted)")
                        .font(.custom("JetBrainsMono-Regular", size: 12))
                        .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                        .padding(.top, 4)
                }
            }

            Button("Close") { dismiss() }
                .font(.custom("Inter-Regular", size: 14))
                .foregroundColor(.white.opacity(0.4))
                .padding(.top, 12)

            // Energy spend option — bypass cooldown for a cost
            if ReefEnergyManager.shared.canAfford(ReefEnergyManager.musicCatchCost) {
                Button(action: {
                    if ReefEnergyManager.shared.spend(ReefEnergyManager.musicCatchCost) {
                        openMusicPicker()
                    }
                }) {
                    VStack(spacing: 4) {
                        Text("Spend \(ReefEnergyManager.musicCatchCost)⚡ for another catch")
                            .font(.custom("Inter-Medium", size: 13))
                            .foregroundColor(DesignTokens.xoGold)
                        Text("You have \(ReefEnergyManager.shared.currentEnergy) energy")
                            .font(.custom("JetBrainsMono-Regular", size: 10))
                            .foregroundColor(.white.opacity(0.3))
                    }
                }
                .padding(.top, 8)
            }

            Spacer()
        }
    }

    @State private var showMusicPicker = false
    @State private var showDeniedAlert = false

    private func openMusicPicker() {
        // Check music library authorization
        let status = MPMediaLibrary.authorizationStatus()
        switch status {
        case .authorized:
            showMusicPicker = true
        case .notDetermined:
            MPMediaLibrary.requestAuthorization { newStatus in
                DispatchQueue.main.async {
                    if newStatus == .authorized {
                        showMusicPicker = true
                    } else {
                        showDeniedAlert = true
                    }
                }
            }
        default:
            showDeniedAlert = true
        }
    }

    // MARK: - Phase 2: Hash Animation

    private var hashAnimationPhase: some View {
        VStack(spacing: 24) {
            Spacer()

            if let song = selectedSong {
                // Song info
                VStack(spacing: 4) {
                    Text(song.title)
                        .font(.custom("SpaceGrotesk-Bold", size: 18))
                        .foregroundColor(.white)
                        .lineLimit(1)
                    Text(song.artist)
                        .font(.custom("Inter-Regular", size: 14))
                        .foregroundColor(.white.opacity(0.5))
                        .lineLimit(1)
                }
            }

            // Hash visualization — rotating ring of hex characters
            ZStack {
                Circle()
                    .stroke(DesignTokens.reefJade.opacity(0.2), lineWidth: 2)
                    .frame(width: 180, height: 180)

                // Animated progress ring
                Circle()
                    .trim(from: 0, to: CGFloat(hashAnimationProgress))
                    .stroke(DesignTokens.reefJade, lineWidth: 3)
                    .frame(width: 180, height: 180)
                    .rotationEffect(.degrees(-90))

                // Hash hex preview (scrolling characters)
                if let seed = hashSeed {
                    Text(String(seed.hashHex.prefix(12)) + "...")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(DesignTokens.reefJade.opacity(0.7))
                }

                Text("HASHING")
                    .font(.custom("SpaceGrotesk-Bold", size: 14))
                    .tracking(3)
                    .foregroundColor(.white.opacity(0.6))
                    .offset(y: 30)
            }

            Spacer()
        }
    }

    private func beginHashAnimation() {
        guard let song = selectedSong else { return }

        // Compute the hash immediately (it's fast)
        let seed = SpecimenHashGenerator.generateSeed(
            artist: song.artist,
            title: song.title,
            album: song.album,
            duration: song.duration
        )
        hashSeed = seed

        // Generate the specimen
        generatedSpecimen = SpecimenHashGenerator.createSpecimen(
            from: seed,
            trackTitle: song.title,
            trackArtist: song.artist
        )

        // Animate the hash visualization over 2 seconds
        phase = .hashing
        hashAnimationProgress = 0

        withAnimation(.easeInOut(duration: 2.0)) {
            hashAnimationProgress = 1.0
        }

        // Transition to ceremony after 2 seconds
        DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) {
            withAnimation(.easeInOut(duration: 0.3)) {
                phase = .ceremony
            }
        }
    }

    // MARK: - Phase 3: Chest Ceremony

    private var ceremonyPhase: some View {
        Group {
            if let specimen = generatedSpecimen {
                ChestCeremony(
                    creatureName: specimen.name,
                    category: specimen.category,
                    rarity: specimen.rarity,
                    cosmeticTier: specimen.cosmeticTier,
                    subtypeID: specimen.subtype,
                    isFirstEncounter: true, // TODO: check collection for prior encounters
                    onComplete: {
                        withAnimation(.easeInOut(duration: 0.3)) {
                            phase = .card
                        }
                    }
                )
            }
        }
    }

    // MARK: - Phase 4: Creature Card

    private var cardPhase: some View {
        VStack(spacing: 20) {
            if let specimen = generatedSpecimen {
                ScrollView {
                    CreatureCard(specimen: specimen)
                        .padding(.horizontal, 20)
                        .padding(.top, 20)
                }

                // Add to Reef button
                Button(action: addToReef) {
                    Text("ADD TO REEF")
                        .font(.custom("SpaceGrotesk-Bold", size: 16))
                        .tracking(2)
                        .foregroundColor(.white)
                        .frame(maxWidth: .infinity)
                        .frame(height: 50)
                        .background(
                            RoundedRectangle(cornerRadius: 25)
                                .fill(DesignTokens.reefJade)
                        )
                }
                .padding(.horizontal, 40)

                Button("Release") {
                    dismiss()
                }
                .font(.custom("Inter-Regular", size: 14))
                .foregroundColor(.white.opacity(0.3))
                .padding(.bottom, 16)
            }
        }
    }

    private func addToReef() {
        guard let specimen = generatedSpecimen else { return }
        if let _ = reefStore.addSpecimen(specimen) {
            reefStore.save()
            firstLaunchManager.recordMusicCatch()
            ReefStatsTracker.shared.increment(.musicCatches)
            dismiss()
        } else {
            showReefFullAlert = true
        }
    }
}

// MARK: - Supporting Types

/// Phases of the music catch flow
enum MusicCatchPhase {
    case picker    // Choosing a song
    case hashing   // Hash animation playing
    case ceremony  // Chest opening ceremony
    case card      // Creature card reveal
}

/// Song metadata extracted from MPMediaItem
struct SongMetadata {
    let artist: String
    let title: String
    let album: String
    let duration: TimeInterval
}
