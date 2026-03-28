import SwiftUI

/// Reef header bar: tappable reef name + single overflow menu.
/// Secondary actions (theme, presets, export, gallery, clear wires) are collapsed
/// into an ellipsis menu — keeping the header to 2 elements so the reef grid gets max space.
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
                // Reef name — the primary element; taps to rename
                Button(action: {
                    editingReefName = reefStore.reefName
                    showReefRename = true
                }) {
                    Text(reefStore.reefName)
                        .font(DesignTokens.heading(18))
                        .foregroundColor(.white)
                }

                Spacer()

                // Journey progress replaces depth counter until the tutorial is complete
                if !firstLaunchManager.isJourneyComplete {
                    Text("Journey: \(firstLaunchManager.journeyStep)/8")
                        .font(DesignTokens.mono(11))
                        .foregroundColor(DesignTokens.xoGold)
                } else {
                    Text("Depth: \(reefStore.totalDiveDepth)m")
                        .font(DesignTokens.mono(11))
                        .foregroundColor(DesignTokens.mutedText)
                }

                // Overflow menu — all secondary actions
                Menu {
                    // Theme submenu
                    Menu("Theme") {
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
                    }

                    // Preset save / load
                    Button(action: { showSaveDialog = true }) {
                        Label("Save Preset", systemImage: "square.and.arrow.down")
                    }
                    if !presetManager.presets.isEmpty {
                        Button(action: { showLoadSheet = true }) {
                            Label("Load Preset", systemImage: "list.bullet")
                        }
                    }

                    Divider()

                    // Export / share
                    Button(action: {
                        if let url = XOReefExporter.exportToFile(reefStore: reefStore) {
                            reefExportURL = url
                            showReefExport = true
                        }
                    }) {
                        Label("Export .xoreef", systemImage: "square.and.arrow.up")
                    }
                    Button(action: {
                        galleryImage = ReefGalleryGenerator.generate(reefStore: reefStore)
                        showGalleryShare = true
                    }) {
                        Label("Share Gallery", systemImage: "photo.artframe")
                    }

                    // Clear wires — destructive, only shown when wires exist
                    if !reefStore.couplingRoutes.isEmpty {
                        Divider()

                        Button(role: .destructive, action: {
                            reefStore.couplingRoutes.removeAll()
                            reefStore.save()
                            audioEngine.applyReefConfiguration(reefStore)
                        }) {
                            Label("Clear Wires", systemImage: "link.badge.plus")
                        }
                    }
                } label: {
                    Image(systemName: "ellipsis.circle")
                        .font(.system(size: 16))
                        .foregroundColor(.white.opacity(0.4))
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
