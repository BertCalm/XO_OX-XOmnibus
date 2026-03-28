import SwiftUI
import CoreMIDI

// MARK: - CollectionTab

/// Pokédex-style view of all specimen types.
/// Shows 16 core specimens (discoverable through normal spawns) and
/// 8 deep specimens (locked, showing unlock conditions).
struct CollectionTab: View {
    @EnvironmentObject var reefStore: ReefStore

    @State private var selectedSpecimen: Specimen?
    @State private var showingCard = false
    @State private var deepSectionExpanded = false
    @State private var showResetConfirm = false
    @State private var notificationsEnabled = true
    @State private var showStats = false
    @State private var showHelp = false
    @State private var showProfile = false
    @StateObject private var milestoneManager = MilestoneManager()

    @StateObject private var collectionTracker = CollectionTracker()
    @State private var pendingRewards: [CollectionMilestone] = []
    @State private var showRewardAlert = false
    @StateObject private var tradePost = TradePostManager()
    @ObservedObject private var masteryManager = MasteryManager.shared

    // .xoreef export
    @State private var exportURL: URL?
    @State private var showExportShare = false

    // OSC output
    @ObservedObject private var oscSender = OSCSender.shared
    @State private var oscHostInput = ""
    @State private var showOSCConfig = false

    // Compare mode
    @State private var compareMode = false
    @State private var compareSpecimenA: Specimen?
    @State private var compareSpecimenB: Specimen?
    @State private var showCompare = false

    // Compute the set of discovered subtype IDs once per render.
    // A type is "discovered" if found in the reef OR in the full collection.
    private var discoveredSubtypes: Set<String> {
        // Reef specimens (currently equipped)
        let reefSubtypes = reefStore.specimens
            .compactMap { $0?.subtype }

        // All specimens from persistent collection (reef + stasis + collection)
        let allSubtypes = reefStore.loadAllSpecimens().map { $0.subtype }

        return Set(reefSubtypes + allSubtypes)
    }

    private var discoveredCoreCount: Int {
        let subtypes = discoveredSubtypes
        return SpecimenCatalog.coreSpecimens.filter { subtypes.contains($0.subtypeID) }.count
    }

    private var discoveredDeepCount: Int {
        let subtypes = discoveredSubtypes
        return SpecimenCatalog.deepSpecimens.filter { subtypes.contains($0.subtypeID) }.count
    }

    /// Find the best matching Specimen for a catalog entry's subtypeID.
    /// Checks the live reef first, then falls back to the full persistent collection.
    private func specimen(for entry: CatalogEntry) -> Specimen? {
        reefStore.specimens.compactMap { $0 }.first { $0.subtype == entry.subtypeID }
            ?? reefStore.loadAllSpecimens().first { $0.subtype == entry.subtypeID }
    }

    /// Central tap handler — branches on compareMode.
    private func handleDiscoveredTap(entry: CatalogEntry) {
        guard let resolvedSpecimen = specimen(for: entry) else { return }
        if compareMode {
            if compareSpecimenA == nil {
                compareSpecimenA = resolvedSpecimen
            } else if compareSpecimenB == nil {
                compareSpecimenB = resolvedSpecimen
                showCompare = true
            }
        } else {
            selectedSpecimen = resolvedSpecimen
            showingCard = true
        }
    }

    // Split core entries by category (only core specimens for normal sections)
    private var coreSources: [CatalogEntry] {
        SpecimenCatalog.sources.filter { !$0.isDeepSpecimen }
    }
    private var coreProcessors: [CatalogEntry] {
        SpecimenCatalog.processors.filter { !$0.isDeepSpecimen }
    }
    private var coreModulators: [CatalogEntry] {
        SpecimenCatalog.modulators.filter { !$0.isDeepSpecimen }
    }
    private var coreEffects: [CatalogEntry] {
        SpecimenCatalog.effects.filter { !$0.isDeepSpecimen }
    }

    var body: some View {
        NavigationView {
            ZStack {
                DesignTokens.background
                    .ignoresSafeArea()

                ScrollView {
                    VStack(alignment: .leading, spacing: 24) {
                        // Header
                        collectionHeader

                        // Favorites section (if any)
                        favoritesSection

                        // Quick stats
                        HStack(spacing: 16) {
                            statBubble(value: "\(reefStore.specimens.compactMap { $0 }.count)", label: "In Reef")
                            statBubble(value: "\(reefStore.loadStasisSpecimens().count)", label: "In Stasis")
                            statBubble(value: "\(reefStore.totalDiveDepth)m", label: "Depth")
                            statBubble(value: "\(milestoneManager.unlockedCount)", label: "Milestones")
                        }
                        .padding(.horizontal, 20)
                        .padding(.bottom, 8)

                        // MARK: Mastery / Prestige Display
                        masterySection

                        let subtypes = discoveredSubtypes

                        // MARK: Core Sections
                        Text("CORE SPECIMENS")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.35))
                            .tracking(1.5)

                        CollectionSection(
                            title: "Shells",
                            categoryColor: DesignTokens.sourceColor,
                            entries: coreSources,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in handleDiscoveredTap(entry: entry) }
                        )

                        // Underwater chain separator — subtle texture between category sections
                        if let chain = UIImage(named: "UIChain") {
                            Image(uiImage: chain)
                                .resizable()
                                .interpolation(.none)
                                .frame(height: 12)
                                .opacity(0.08)
                                .padding(.horizontal, 60)
                        }

                        CollectionSection(
                            title: "Coral",
                            categoryColor: DesignTokens.errorRed,
                            entries: coreProcessors,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in handleDiscoveredTap(entry: entry) }
                        )

                        // Underwater chain separator
                        if let chain = UIImage(named: "UIChain") {
                            Image(uiImage: chain)
                                .resizable()
                                .interpolation(.none)
                                .frame(height: 12)
                                .opacity(0.08)
                                .padding(.horizontal, 60)
                        }

                        CollectionSection(
                            title: "Currents",
                            categoryColor: DesignTokens.modulatorColor,
                            entries: coreModulators,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in handleDiscoveredTap(entry: entry) }
                        )

                        // Underwater chain separator
                        if let chain = UIImage(named: "UIChain") {
                            Image(uiImage: chain)
                                .resizable()
                                .interpolation(.none)
                                .frame(height: 12)
                                .opacity(0.08)
                                .padding(.horizontal, 60)
                        }

                        CollectionSection(
                            title: "Tide Pools",
                            categoryColor: DesignTokens.effectColor,
                            entries: coreEffects,
                            discoveredSubtypes: subtypes,
                            onDiscoveredTap: { entry in handleDiscoveredTap(entry: entry) }
                        )

                        // MARK: Deep Specimens Section (collapsible)
                        deepSpecimensSection(discoveredSubtypes: subtypes)

                        // MARK: Collection Progress Section
                        collectionProgressSection

                        // MARK: Milestones Section
                        milestonesSection

                        // MARK: Trading Post Section
                        tradingPostSection

                        // MARK: Settings Section
                        settingsSection
                    }
                    .padding(.horizontal, 16)
                    .padding(.bottom, 32)
                }
                .onAppear {
                    collectionTracker.refresh(reefStore: reefStore)
                    masteryManager.updateFromReef(reefStore: reefStore)
                }
            }
            .navigationBarHidden(true)
            .alert("Reset All Data?", isPresented: $showResetConfirm) {
                Button("Reset Everything", role: .destructive) {
                    // Clear UserDefaults (streaks, badges, milestones, energy, etc.)
                    let domain = Bundle.main.bundleIdentifier!
                    UserDefaults.standard.removePersistentDomain(forName: domain)

                    // Clear all specimens (reef + stasis + collection) and wipe GRDB
                    reefStore.resetAll()
                }
                Button("Cancel", role: .cancel) {}
            } message: {
                Text("This will delete all specimens, wiring, progress, and settings. This cannot be undone.")
            }
            .alert("OSC Output", isPresented: $showOSCConfig) {
                TextField("IP Address (e.g., 192.168.1.100)", text: $oscHostInput)
                Button("Connect") {
                    oscSender.targetHost = oscHostInput
                    oscSender.connect()
                }
                Button("Disconnect") { oscSender.disconnect() }
                Button("Cancel", role: .cancel) {}
            }
            .sheet(isPresented: $showingCard) {
                if let specimen = selectedSpecimen {
                    MicroscopeView(specimen: specimen)
                        .background(DesignTokens.background.ignoresSafeArea())
                }
            }
            .sheet(isPresented: $showExportShare) {
                if let url = exportURL {
                    ShareSheet(items: [url])
                }
            }
            .sheet(isPresented: $showCompare, onDismiss: {
                compareSpecimenA = nil
                compareSpecimenB = nil
            }) {
                if let a = compareSpecimenA, let b = compareSpecimenB {
                    CompareView(specimenA: a, specimenB: b)
                }
            }
            .sheet(isPresented: $showStats) {
                StatsView()
            }
            .sheet(isPresented: $showHelp) {
                HelpView()
            }
            .sheet(isPresented: $showProfile) {
                NavigationView {
                    ProfileView()
                        .environmentObject(reefStore)
                        .navigationTitle("Profile")
                        .navigationBarTitleDisplayMode(.inline)
                        .toolbar {
                            ToolbarItem(placement: .cancellationAction) {
                                Button("Done") { showProfile = false }
                            }
                        }
                }
            }
        }
        .navigationViewStyle(.stack)
    }

    // MARK: - Header

    private var collectionHeader: some View {
        VStack(alignment: .leading, spacing: 4) {
            HStack(alignment: .lastTextBaseline, spacing: 8) {
                Text("Collection")
                    .font(.custom("SpaceGrotesk-Bold", size: 20, relativeTo: .title2))
                    .foregroundColor(.white)
                Spacer()
                Button(action: { showProfile = true }) {
                    Image(systemName: "person.circle")
                        .font(.system(size: 14))
                        .foregroundColor(.white.opacity(0.4))
                }
                .accessibilityLabel("Reef Profile")
                Button(action: { showStats = true }) {
                    Image(systemName: "chart.bar")
                        .font(.system(size: 12))
                        .foregroundColor(.white.opacity(0.4))
                }
                .accessibilityLabel("Reef Statistics")
                Button(action: {
                    compareMode.toggle()
                    if !compareMode {
                        compareSpecimenA = nil
                        compareSpecimenB = nil
                    }
                }) {
                    Text(compareMode ? "Cancel" : "Compare")
                        .font(.custom("Inter-Regular", size: 11))
                        .foregroundColor(compareMode ? DesignTokens.errorRed : DesignTokens.reefJade.opacity(0.6))
                }
                .accessibilityLabel(compareMode ? "Cancel compare" : "Compare specimens")
            }
            HStack(spacing: 12) {
                // Core count
                HStack(spacing: 4) {
                    Text("Core")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.4))
                    Text("\(discoveredCoreCount) / 16")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(DesignTokens.reefJade)
                }
                // Deep count
                HStack(spacing: 4) {
                    Text("Deep")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(.white.opacity(0.4))
                    Text("\(discoveredDeepCount) / 8")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .foregroundColor(DesignTokens.deepAccent)
                }
                Spacer()
            }
        }
        .padding(.top, 16)
    }

    // MARK: - Favorites Section

    @ViewBuilder
    private var favoritesSection: some View {
        let favorites = reefStore.specimens.compactMap { $0 }.filter { $0.isFavorite }
        if !favorites.isEmpty {
            VStack(alignment: .leading, spacing: 8) {
                HStack {
                    Image(systemName: "heart.fill")
                        .foregroundColor(DesignTokens.errorRed)
                        .font(.system(size: 10))
                    Text("FAVORITES")
                        .font(.custom("JetBrainsMono-Regular", size: 10))
                        .tracking(1.5)
                        .foregroundColor(DesignTokens.errorRed)
                }
                .padding(.horizontal, 20)

                ScrollView(.horizontal, showsIndicators: false) {
                    HStack(spacing: 12) {
                        ForEach(favorites) { specimen in
                            Button(action: {
                                selectedSpecimen = specimen
                                showingCard = true
                            }) {
                                VStack(spacing: 4) {
                                    SpecimenSprite(subtype: specimen.subtype, category: specimen.category, size: 40)
                                    Text(specimen.creatureName)
                                        .font(.custom("Inter-Regular", size: 8))
                                        .foregroundColor(.white.opacity(0.6))
                                        .lineLimit(1)
                                }
                            }
                        }
                    }
                    .padding(.horizontal, 20)
                }
            }
            .padding(.bottom, 8)
        }
    }

    // MARK: - Mastery / Prestige Section

    @ViewBuilder
    private var masterySection: some View {
        if masteryManager.isPrestigeUnlocked {
            VStack(spacing: 4) {
                HStack(spacing: 6) {
                    Image(systemName: "crown.fill")
                        .font(.system(size: 12))
                        .foregroundColor(DesignTokens.xoGold)
                    Text(masteryManager.masteryTitle)
                        .font(.custom("SpaceGrotesk-Bold", size: 14))
                        .foregroundColor(DesignTokens.xoGold)
                    Text("Lv.\(masteryManager.masteryLevel)")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(.white.opacity(0.5))
                }

                // Mastery XP bar
                GeometryReader { geo in
                    ZStack(alignment: .leading) {
                        RoundedRectangle(cornerRadius: 2)
                            .fill(Color.white.opacity(0.06))
                        RoundedRectangle(cornerRadius: 2)
                            .fill(DesignTokens.xoGold.opacity(0.4))
                            .frame(width: geo.size.width * CGFloat(masteryManager.progressToNextLevel))
                    }
                }
                .frame(height: 4)
                .padding(.horizontal, 60)
            }
            .padding(.vertical, 4)
        } else {
            let maxedCount = reefStore.specimens.compactMap { $0 }.filter { $0.level >= 10 }.count
            if maxedCount > 0 {
                Text("Prestige: \(maxedCount)/16 specimens at Lv.10")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.25))
                    .padding(.horizontal, 20)
            }
        }
    }

    // MARK: - Collection Progress Section

    private var collectionProgressSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            Text("COLLECTION PROGRESS")
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .tracking(1.5)
                .foregroundColor(DesignTokens.xoGold)
                .padding(.horizontal, 4)

            ForEach(collectionTracker.milestones) { milestone in
                let prog = collectionTracker.progress(for: milestone)
                let complete = prog >= milestone.requiredCount
                let claimed = collectionTracker.isClaimed(milestone.id)

                HStack(spacing: 8) {
                    Image(systemName: claimed ? "checkmark.seal.fill" : (complete ? "seal.fill" : "circle"))
                        .font(.system(size: 12))
                        .foregroundColor(claimed ? DesignTokens.reefJade : (complete ? DesignTokens.xoGold : .white.opacity(0.2)))

                    VStack(alignment: .leading, spacing: 1) {
                        Text(milestone.title)
                            .font(.custom("SpaceGrotesk-Bold", size: 12))
                            .foregroundColor(claimed ? .white.opacity(0.4) : .white.opacity(0.7))
                        Text("\(prog)/\(milestone.requiredCount) — \(milestone.reward)")
                            .font(.custom("Inter-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.25))
                    }

                    Spacer()

                    if complete && !claimed {
                        Button("Claim") {
                            collectionTracker.claim(milestone)
                        }
                        .font(.custom("JetBrainsMono-Bold", size: 9))
                        .foregroundColor(DesignTokens.xoGold)
                    }
                }
                .padding(.horizontal, 4)
                .padding(.vertical, 4)
            }
        }
    }

    // MARK: - Milestones Section

    private var milestonesSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Text("MILESTONES")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .tracking(1.5)
                    .foregroundColor(DesignTokens.xoGold)
                Spacer()
                Text("\(milestoneManager.unlockedCount)/\(milestoneManager.totalCount)")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.3))
            }
            .padding(.horizontal, 4)

            ForEach(milestoneManager.milestones) { milestone in
                HStack(spacing: 10) {
                    Image(systemName: milestone.icon)
                        .font(.system(size: 14))
                        .foregroundColor(milestone.unlocked ? DesignTokens.xoGold : .white.opacity(0.2))
                        .frame(width: 24)

                    VStack(alignment: .leading, spacing: 2) {
                        Text(milestone.title)
                            .font(.custom("SpaceGrotesk-Bold", size: 12))
                            .foregroundColor(milestone.unlocked ? .white : .white.opacity(0.5))
                        Text(milestone.description)
                            .font(.custom("Inter-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.3))
                    }

                    Spacer()

                    if milestone.unlocked {
                        Image(systemName: "checkmark.circle.fill")
                            .foregroundColor(DesignTokens.reefJade)
                    } else {
                        Text("\(milestone.progress)/\(milestone.requirement)")
                            .font(.custom("JetBrainsMono-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.3))
                    }
                }
                .padding(.horizontal, 4)
                .padding(.vertical, 4)
            }
        }
    }

    // MARK: - Trading Post Section

    private var tradingPostSection: some View {
        VStack(alignment: .leading, spacing: 8) {
            HStack {
                Image(systemName: "arrow.left.arrow.right")
                    .foregroundColor(DesignTokens.reefJade)
                    .font(.system(size: 10))
                Text("TRADING POST")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .tracking(1.5)
                    .foregroundColor(DesignTokens.reefJade)
                Spacer()
                Text("\(tradePost.offers.count) offers")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.3))
            }
            .padding(.horizontal, 20)

            ForEach(tradePost.offers) { offer in
                let requestedName = SpecimenCatalog.entry(for: offer.requestedSubtype)?.creatureName ?? offer.requestedSubtype
                let offeredName = SpecimenCatalog.entry(for: offer.offeredSubtype)?.creatureName ?? offer.offeredSubtype
                let requestedCategory = SpecimenCatalog.entry(for: offer.requestedSubtype)?.category ?? .source
                let offeredCategory = SpecimenCatalog.entry(for: offer.offeredSubtype)?.category ?? .source

                HStack(spacing: 8) {
                    // What they want
                    VStack(spacing: 2) {
                        Text("WANT")
                            .font(.custom("JetBrainsMono-Regular", size: 7))
                            .foregroundColor(.white.opacity(0.2))
                        SpecimenSprite(subtype: offer.requestedSubtype, category: requestedCategory, size: 24)
                        Text(requestedName)
                            .font(.custom("Inter-Regular", size: 8))
                            .foregroundColor(.white.opacity(0.5))
                        Text("Lv.\(offer.requestedMinLevel)+")
                            .font(.custom("JetBrainsMono-Regular", size: 7))
                            .foregroundColor(.white.opacity(0.3))
                    }
                    .frame(maxWidth: .infinity)

                    Image(systemName: "arrow.right")
                        .font(.system(size: 10))
                        .foregroundColor(.white.opacity(0.2))

                    // What they offer
                    VStack(spacing: 2) {
                        Text("GIVE")
                            .font(.custom("JetBrainsMono-Regular", size: 7))
                            .foregroundColor(.white.opacity(0.2))
                        SpecimenSprite(subtype: offer.offeredSubtype, category: offeredCategory, size: 24)
                        Text(offeredName)
                            .font(.custom("Inter-Regular", size: 8))
                            .foregroundColor(.white.opacity(0.5))
                        Text("\(offer.offeredRarity.rawValue) Lv.\(offer.offeredLevel)")
                            .font(.custom("JetBrainsMono-Regular", size: 7))
                            .foregroundColor(DesignTokens.xoGold.opacity(0.5))
                    }
                    .frame(maxWidth: .infinity)
                }
                .padding(.vertical, 6)
                .padding(.horizontal, 20)
                .background(Color.white.opacity(0.02))
                .clipShape(RoundedRectangle(cornerRadius: 8))
                .overlay(
                    RoundedRectangle(cornerRadius: 8)
                        .strokeBorder(DesignTokens.reefJade.opacity(0.15), lineWidth: 1)
                )
                .padding(.horizontal, 20)
            }

            if tradePost.offers.isEmpty {
                Text("No active offers — check back tomorrow.")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.2))
                    .padding(.horizontal, 20)
                    .padding(.vertical, 4)
            }
        }
        .padding(.top, 8)
    }

    // MARK: - Settings Section

    private var settingsSection: some View {
        VStack(alignment: .leading, spacing: 12) {
            HStack {
                Text("SETTINGS")
                    .font(.custom("JetBrainsMono-Regular", size: 10))
                    .tracking(1.5)
                    .foregroundColor(.white.opacity(0.2))
                Spacer()
            }
            .padding(.horizontal, 20)
            .padding(.top, 16)

            settingsRow(icon: "waveform", title: "Audio Quality", detail: "Low Latency (5ms)")
            settingsRow(icon: "house.fill", title: "Reef Proximity", detail: "Uses home location")
            settingsRow(icon: "music.note", title: "Daily Music Catches", detail: "1 per day")
            settingsRow(icon: "pianokeys", title: "MIDI Input", detail: "\(MIDIGetNumberOfSources()) sources connected")

            HStack(spacing: 10) {
                Image(systemName: "bell.fill")
                    .font(.system(size: 14))
                    .foregroundColor(.white.opacity(0.3))
                    .frame(width: 24)
                VStack(alignment: .leading, spacing: 2) {
                    Text("Notifications")
                        .font(.custom("Inter-Medium", size: 13))
                        .foregroundColor(.white.opacity(0.6))
                    Text("Daily reminders and energy alerts")
                        .font(.custom("Inter-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.25))
                }
                Spacer()
                Toggle("", isOn: $notificationsEnabled)
                    .labelsHidden()
                    .tint(DesignTokens.reefJade)
                    .onChange(of: notificationsEnabled) { enabled in
                        if enabled {
                            NotificationManager.shared.scheduleDailyEnergyReminder()
                            NotificationManager.shared.scheduleMusicCatchReminder()
                        } else {
                            NotificationManager.shared.cancelAll()
                        }
                        UserDefaults.standard.set(enabled, forKey: "obrix_notifications_enabled")
                    }
            }
            .padding(.horizontal, 20)
            .padding(.vertical, 4)
            .onAppear {
                notificationsEnabled = UserDefaults.standard.bool(forKey: "obrix_notifications_enabled")
                // Default to true if key doesn't exist
                if !UserDefaults.standard.bool(forKey: "obrix_notifications_set") {
                    notificationsEnabled = true
                    UserDefaults.standard.set(true, forKey: "obrix_notifications_set")
                }
            }

            Button(action: {
                oscHostInput = oscSender.targetHost
                showOSCConfig = true
            }) {
                HStack(spacing: 10) {
                    Image(systemName: "wifi")
                        .font(.system(size: 14))
                        .foregroundColor(oscSender.isConnected ? DesignTokens.reefJade : .white.opacity(0.3))
                    VStack(alignment: .leading, spacing: 2) {
                        Text("OSC Output")
                            .font(.custom("Inter-Medium", size: 13))
                            .foregroundColor(.white.opacity(0.6))
                        Text(oscSender.isConnected ? "Connected to \(oscSender.targetHost)" : "Not connected")
                            .font(.custom("Inter-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.25))
                    }
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.vertical, 4)
            }

            Button(action: {
                if let url = XOReefExporter.exportToFile(reefStore: reefStore) {
                    exportURL = url
                    showExportShare = true
                }
            }) {
                HStack(spacing: 10) {
                    Image(systemName: "square.and.arrow.up")
                        .font(.system(size: 14))
                        .foregroundColor(DesignTokens.reefJade.opacity(0.6))
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Export Reef (.xoreef)")
                            .font(.custom("Inter-Medium", size: 13))
                            .foregroundColor(DesignTokens.reefJade.opacity(0.7))
                        Text("Share or import to desktop XOceanus")
                            .font(.custom("Inter-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.25))
                    }
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.vertical, 4)
            }

            Button(action: { showHelp = true }) {
                settingsRow(icon: "questionmark.circle", title: "Help & Guide", detail: "\(HelpContent.topics.count) topics")
            }

            Button(action: { showResetConfirm = true }) {
                HStack(spacing: 10) {
                    Image(systemName: "exclamationmark.triangle.fill")
                        .font(.system(size: 14))
                        .foregroundColor(DesignTokens.errorRed.opacity(0.5))
                    VStack(alignment: .leading, spacing: 2) {
                        Text("Reset All Data")
                            .font(.custom("Inter-Medium", size: 13))
                            .foregroundColor(DesignTokens.errorRed.opacity(0.6))
                        Text("Delete all specimens, wiring, and progress")
                            .font(.custom("Inter-Regular", size: 9))
                            .foregroundColor(.white.opacity(0.2))
                    }
                    Spacer()
                }
                .padding(.horizontal, 20)
                .padding(.vertical, 4)
            }

            VStack(spacing: 4) {
                Text("OBRIX Pocket")
                    .font(.custom("SpaceGrotesk-Bold", size: 12))
                    .foregroundColor(.white.opacity(0.3))
                Text("by XO_OX Designs")
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.2))
                Text("v1.0.0 · Build 75 · The reef remembers.")
                    .font(.custom("JetBrainsMono-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.15))
                    .italic()
            }
            .frame(maxWidth: .infinity)
            .padding(.top, 20)
            .padding(.bottom, 32)
        }
    }

    private func settingsRow(icon: String, title: String, detail: String) -> some View {
        HStack(spacing: 10) {
            Image(systemName: icon)
                .font(.system(size: 14))
                .foregroundColor(.white.opacity(0.3))
                .frame(width: 24)
            VStack(alignment: .leading, spacing: 2) {
                Text(title)
                    .font(.custom("Inter-Medium", size: 13))
                    .foregroundColor(.white.opacity(0.6))
                Text(detail)
                    .font(.custom("Inter-Regular", size: 9))
                    .foregroundColor(.white.opacity(0.25))
            }
            Spacer()
        }
        .padding(.horizontal, 20)
        .padding(.vertical, 4)
    }

    private func statBubble(value: String, label: String) -> some View {
        VStack(spacing: 2) {
            Text(value)
                .font(.custom("JetBrainsMono-Bold", size: 16))
                .foregroundColor(.white)
            Text(label)
                .font(.custom("Inter-Regular", size: 8))
                .foregroundColor(.white.opacity(0.3))
        }
        .frame(maxWidth: .infinity)
    }

    // MARK: - Deep Specimens Section

    @ViewBuilder
    private func deepSpecimensSection(discoveredSubtypes: Set<String>) -> some View {
        VStack(alignment: .leading, spacing: 10) {
            // Collapsible header
            Button(action: {
                withAnimation(.easeInOut(duration: 0.2)) {
                    deepSectionExpanded.toggle()
                }
            }) {
                HStack(spacing: 8) {
                    Image(systemName: "lock.fill")
                        .font(.system(size: 9, weight: .medium))
                        .foregroundColor(DesignTokens.deepAccent)

                    Text("DEEP SPECIMENS")
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(DesignTokens.deepAccent)
                        .tracking(1.5)

                    Text("(\(discoveredDeepCount)/8 unlocked)")
                        .font(.custom("JetBrainsMono-Regular", size: 9))
                        .foregroundColor(.white.opacity(0.3))
                        .tracking(0.8)

                    Spacer()

                    Image(systemName: deepSectionExpanded ? "chevron.up" : "chevron.down")
                        .font(.system(size: 10, weight: .medium))
                        .foregroundColor(.white.opacity(0.3))
                }
            }
            .buttonStyle(.plain)

            if deepSectionExpanded {
                DeepSpecimensGrid(
                    entries: SpecimenCatalog.deepSpecimens,
                    discoveredSubtypes: discoveredSubtypes,
                    onDiscoveredTap: { entry in handleDiscoveredTap(entry: entry) }
                )
                .transition(.opacity.combined(with: .move(edge: .top)))
            }
        }
    }
}

// MARK: - CollectionSection

/// A single category section with a colored header and 4-column card grid.
private struct CollectionSection: View {
    let title: String
    let categoryColor: Color
    let entries: [CatalogEntry]
    let discoveredSubtypes: Set<String>
    /// Called when a discovered card is tapped. Passes the tapped entry for specimen lookup.
    var onDiscoveredTap: ((CatalogEntry) -> Void)? = nil

    private let columns = Array(repeating: GridItem(.flexible(), spacing: 8), count: 4)

    var body: some View {
        VStack(alignment: .leading, spacing: 10) {
            // Section header
            Text(title.uppercased())
                .font(.custom("JetBrainsMono-Regular", size: 10))
                .foregroundColor(categoryColor)
                .tracking(1.2)

            LazyVGrid(columns: columns, spacing: 8) {
                ForEach(entries, id: \.subtypeID) { entry in
                    let isDiscovered = discoveredSubtypes.contains(entry.subtypeID)
                    CollectionCard(
                        entry: entry,
                        discovered: isDiscovered
                    )
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white.opacity(0.03))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .strokeBorder(
                                        isDiscovered
                                            ? categoryColor.opacity(0.30)
                                            : Color.white.opacity(0.06),
                                        lineWidth: 1
                                    )
                            )
                    )
                    .onTapGesture {
                        guard isDiscovered else { return }
                        onDiscoveredTap?(entry)
                    }
                }
            }
        }
    }
}

// MARK: - DeepSpecimensGrid

/// Grid showing all 8 deep specimens. Unlocked ones show normally;
/// locked ones show a lock icon and unlock condition instead of "???".
private struct DeepSpecimensGrid: View {
    let entries: [CatalogEntry]
    let discoveredSubtypes: Set<String>
    var onDiscoveredTap: ((CatalogEntry) -> Void)? = nil

    private let columns = Array(repeating: GridItem(.flexible(), spacing: 8), count: 4)

    var body: some View {
        LazyVGrid(columns: columns, spacing: 8) {
            ForEach(entries, id: \.subtypeID) { entry in
                let isDiscovered = discoveredSubtypes.contains(entry.subtypeID)
                DeepSpecimenCard(entry: entry, discovered: isDiscovered)
                    .background(
                        RoundedRectangle(cornerRadius: 8)
                            .fill(Color.white.opacity(0.02))
                            .overlay(
                                RoundedRectangle(cornerRadius: 8)
                                    .strokeBorder(
                                        isDiscovered
                                            ? DesignTokens.deepAccent.opacity(0.40)
                                            : DesignTokens.deepAccent.opacity(0.12),
                                        lineWidth: 1
                                    )
                            )
                    )
                    .onTapGesture {
                        guard isDiscovered else { return }
                        onDiscoveredTap?(entry)
                    }
            }
        }
    }
}

// MARK: - DeepSpecimenCard

/// Card for deep specimens. Unlocked = sprite + name; locked = lock icon + unlock condition.
private struct DeepSpecimenCard: View {
    let entry: CatalogEntry
    let discovered: Bool

    var body: some View {
        VStack(spacing: 4) {
            if discovered {
                SpecimenSprite(subtype: entry.subtypeID, category: entry.category, size: 48)
                Text(entry.creatureName)
                    .font(.custom("Inter-Regular", size: 10))
                    .foregroundColor(.white.opacity(0.7))
                    .lineLimit(1)
            } else {
                // Lock icon + unlock condition
                ZStack {
                    Circle()
                        .fill(DesignTokens.deepAccent.opacity(0.08))
                        .frame(width: 48, height: 48)
                    Image(systemName: "lock.fill")
                        .font(.system(size: 16, weight: .medium))
                        .foregroundColor(DesignTokens.deepAccent.opacity(0.5))
                }
                Text(entry.unlockCondition)
                    .font(.custom("Inter-Regular", size: 8))
                    .foregroundColor(DesignTokens.deepAccent.opacity(0.55))
                    .lineLimit(2)
                    .multilineTextAlignment(.center)
                    .minimumScaleFactor(0.7)
            }
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
        .padding(.horizontal, 4)
    }
}

// MARK: - CollectionCard

/// A single specimen-type card. Discovered = sprite + name; undiscovered = silhouette + "???".
struct CollectionCard: View {
    let entry: CatalogEntry
    let discovered: Bool

    var body: some View {
        VStack(spacing: 6) {
            if discovered {
                SpecimenSprite(subtype: entry.subtypeID, category: entry.category, size: 48)
            } else {
                // Dark silhouette
                ZStack {
                    Circle()
                        .fill(Color.white.opacity(0.04))
                        .frame(width: 48, height: 48)
                    Text("?")
                        .font(.custom("SpaceGrotesk-Bold", size: 20))
                        .foregroundColor(.white.opacity(0.15))
                }
            }

            Text(discovered ? entry.creatureName : "???")
                .font(.custom("Inter-Regular", size: 10))
                .foregroundColor(discovered ? .white.opacity(0.7) : .white.opacity(0.2))
                .lineLimit(1)

            if discovered {
                Text(entry.personalityLine)
                    .font(.custom("Inter-Regular", size: 8))
                    .foregroundColor(.white.opacity(0.25))
                    .italic()
                    .lineLimit(1)
                    .minimumScaleFactor(0.8)
            }
        }
        .frame(maxWidth: .infinity)
        .padding(.vertical, 8)
    }
}

// MARK: - Preview

#if DEBUG
private func makePreviewStore() -> ReefStore {
    let store = ReefStore()
    store.specimens[0] = Specimen(
        id: UUID(),
        name: "Sawfin",
        category: .source,
        rarity: .common,
        health: 80,
        isPhantom: false,
        phantomScar: false,
        subtype: "polyblep-saw",
        catchAccelPattern: [],
        provenance: [],
        spectralDNA: [],
        parameterState: [:],
        catchLatitude: nil,
        catchLongitude: nil,
        catchTimestamp: Date(),
        catchWeatherDescription: nil,
        creatureGenomeData: nil,
        cosmeticTier: .standard,
        morphIndex: 0,
        musicHash: nil,
        sourceTrackTitle: nil,
        xp: 0,
        level: 1,
        aggressiveScore: 0,
        gentleScore: 0,
        totalPlaySeconds: 0,
        journal: [],
        isFavorite: false
    )
    store.specimens[1] = Specimen(
        id: UUID(),
        name: "Echocave",
        category: .effect,
        rarity: .uncommon,
        health: 65,
        isPhantom: false,
        phantomScar: false,
        subtype: "delay-stereo",
        catchAccelPattern: [],
        provenance: [],
        spectralDNA: [],
        parameterState: [:],
        catchLatitude: nil,
        catchLongitude: nil,
        catchTimestamp: Date(),
        catchWeatherDescription: nil,
        creatureGenomeData: nil,
        cosmeticTier: .standard,
        morphIndex: 0,
        musicHash: nil,
        sourceTrackTitle: nil,
        xp: 0,
        level: 1,
        aggressiveScore: 0,
        gentleScore: 0,
        totalPlaySeconds: 0,
        journal: [],
        isFavorite: false
    )
    return store
}

#Preview {
    CollectionTab()
        .environmentObject(makePreviewStore())
}
#endif
