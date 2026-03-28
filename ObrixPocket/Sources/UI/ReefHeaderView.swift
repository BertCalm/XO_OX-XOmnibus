import SwiftUI

/// Reef header bar: tappable reef name, theme picker, gallery share, save/load preset, .xoreef export,
/// clear-wires button, and the underwater decoration accent.
struct ReefHeaderView: View {
    @EnvironmentObject var audioEngine: AudioEngineManager
    @EnvironmentObject var reefStore: ReefStore
    @EnvironmentObject var firstLaunchManager: FirstLaunchManager

    // Owned state — these only matter to the header
    @StateObject private var presetManager = ReefPresetManager()
    @State private var showReefRename = false
    @State private var editingReefName = ""
    @State private var reefTheme: ReefTheme = .ocean
    @State private var showSaveDialog = false
    @State private var showLoadSheet = false
    @State private var presetName = ""
    @State private var showReefExport = false
    @State private var reefExportURL: URL?
    @State private var showGalleryShare = false
    @State private var galleryImage: UIImage?

    // The live scene reference — supplied by ReefTab so theme changes propagate
    var onThemeChanged: ((ReefTheme) -> Void)?

    var body: some View {
        VStack(spacing: 0) {
            HStack {
                // Tappable reef name — opens rename alert
                Button(action: {
                    editingReefName = reefStore.reefName
                    showReefRename = true
                }) {
                    Text(reefStore.reefName)
                        .font(.custom("SpaceGrotesk-Bold", size: 18))
                        .foregroundColor(.white)
                }

                // Theme picker
                Menu {
                    ForEach(ReefTheme.allCases, id: \.self) { theme in
                        Button(action: {
                            reefTheme = theme
                            onThemeChanged?(theme)
                            UserDefaults.standard.set(theme.rawValue, forKey: "obrix_reef_theme")
                        }) {
                            HStack {
                                Text(theme.rawValue)
                                if theme == reefTheme {
                                    Image(systemName: "checkmark")
                                }
                            }
                        }
                    }
                } label: {
                    Image(systemName: "paintpalette")
                        .font(.system(size: 12))
                        .foregroundColor(.white.opacity(0.3))
                }

                // Gallery share button — generates a reef portrait image
                Button(action: {
                    galleryImage = ReefGalleryGenerator.generate(reefStore: reefStore)
                    showGalleryShare = true
                }) {
                    Image(systemName: "photo.artframe")
                        .font(.system(size: 12))
                        .foregroundColor(.white.opacity(0.3))
                }

                Spacer()

                // Journey progress replaces depth counter until the tutorial is complete
                if !firstLaunchManager.isJourneyComplete {
                    Text("Journey: \(firstLaunchManager.journeyStep)/8")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(DesignTokens.xoGold)
                } else {
                    Text("Depth: \(reefStore.totalDiveDepth)m")
                        .font(.custom("JetBrainsMono-Regular", size: 11))
                        .foregroundColor(DesignTokens.mutedText)
                }

                // Save / Load preset buttons + .xoreef export
                HStack(spacing: 8) {
                    Button(action: { showSaveDialog = true }) {
                        Image(systemName: "square.and.arrow.down")
                            .font(.system(size: 12))
                            .foregroundColor(DesignTokens.reefJade.opacity(0.6))
                    }
                    if !presetManager.presets.isEmpty {
                        Button(action: { showLoadSheet = true }) {
                            Image(systemName: "list.bullet")
                                .font(.system(size: 12))
                                .foregroundColor(DesignTokens.reefJade.opacity(0.6))
                        }
                    }
                    Button(action: {
                        if let url = XOReefExporter.exportToFile(reefStore: reefStore) {
                            reefExportURL = url
                            showReefExport = true
                        }
                    }) {
                        Image(systemName: "square.and.arrow.up")
                            .font(.system(size: 12))
                            .foregroundColor(DesignTokens.reefJade.opacity(0.5))
                    }
                }

                if !reefStore.couplingRoutes.isEmpty {
                    Button(action: {
                        reefStore.couplingRoutes.removeAll()
                        reefStore.save()
                        audioEngine.applyReefConfiguration(reefStore)
                    }) {
                        Text("Clear Wires")
                            .font(.custom("Inter-Regular", size: 11))
                            .foregroundColor(DesignTokens.errorRed.opacity(0.6))
                    }
                }
            }
            .padding(.horizontal, 20)
            .padding(.top, 12)
            .padding(.bottom, 8)

            // Underwater header decoration — subtle texture accent between header and reef grid
            if let headerDecor = UIImage(named: "UIHeader") {
                Image(uiImage: headerDecor)
                    .resizable()
                    .interpolation(.none)
                    .frame(height: 8)
                    .opacity(0.1)
                    .padding(.horizontal, 20)
            }
        }
        .onAppear {
            let savedRaw = UserDefaults.standard.string(forKey: "obrix_reef_theme") ?? "Ocean"
            reefTheme = ReefTheme(rawValue: savedRaw) ?? .ocean
        }
        .alert("Rename Reef", isPresented: $showReefRename) {
            TextField("Reef name", text: $editingReefName)
            Button("Save") {
                if !editingReefName.isEmpty {
                    reefStore.reefName = editingReefName
                    reefStore.save()
                }
            }
            Button("Cancel", role: .cancel) {}
        } message: {
            Text("Give your reef a name")
        }
        .alert("Save Reef Preset", isPresented: $showSaveDialog) {
            TextField("Preset name", text: $presetName)
            Button("Save") {
                if !presetName.isEmpty {
                    presetManager.save(name: presetName, from: reefStore)
                    presetName = ""
                }
            }
            Button("Cancel", role: .cancel) { presetName = "" }
        }
        .sheet(isPresented: $showLoadSheet) {
            ReefPresetList(
                presetManager: presetManager,
                reefStore: reefStore,
                onDismiss: { showLoadSheet = false }
            )
        }
        .sheet(isPresented: $showReefExport) {
            if let url = reefExportURL {
                ShareSheet(items: [url])
            }
        }
        .sheet(isPresented: $showGalleryShare) {
            if let image = galleryImage {
                ShareSheet(items: [image])
            }
        }
    }
}
