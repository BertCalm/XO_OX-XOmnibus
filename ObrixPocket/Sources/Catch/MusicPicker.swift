import SwiftUI
import MediaPlayer

// MARK: - MusicPicker

/// UIViewControllerRepresentable wrapper around MPMediaPickerController.
///
/// Presents the native Apple Music library picker and returns song metadata
/// via a callback. Handles cancellation gracefully.
///
/// Usage:
/// ```swift
/// MusicPicker(
///     onSongSelected: { artist, title, album, duration in … },
///     onCancel: { … }
/// )
/// ```
struct MusicPicker: UIViewControllerRepresentable {

    // MARK: - Inputs

    /// Called when the user selects a song.
    /// Parameters: (artist, title, album, duration in seconds)
    let onSongSelected: (String, String, String, TimeInterval) -> Void

    /// Called when the user dismisses the picker without selecting a song.
    let onCancel: () -> Void

    // MARK: - UIViewControllerRepresentable

    func makeCoordinator() -> Coordinator {
        Coordinator(onSongSelected: onSongSelected, onCancel: onCancel)
    }

    func makeUIViewController(context: Context) -> MPMediaPickerController {
        let picker = MPMediaPickerController(mediaTypes: .music)
        picker.delegate = context.coordinator
        picker.allowsPickingMultipleItems = false
        picker.showsCloudItems = true
        picker.showsItemsWithProtectedAssets = true
        picker.prompt = "Select a track"

        // Apply dark appearance to match Reef aesthetic (#0E0E10 background,
        // Reef Jade #1E8B7E accent). MPMediaPickerController is a UIKit
        // controller so we set the view's background and the navigation bar
        // tint once the view has loaded.
        picker.view.backgroundColor = UIColor(hex: "0E0E10")
        picker.navigationController?.navigationBar.barTintColor = UIColor(hex: "0E0E10")
        picker.navigationController?.navigationBar.tintColor = UIColor(hex: "1E8B7E")
        picker.navigationController?.navigationBar.titleTextAttributes = [
            .foregroundColor: UIColor.white,
            .font: UIFont(name: "SpaceGrotesk-Bold", size: 17)
                ?? UIFont.boldSystemFont(ofSize: 17)
        ]

        return picker
    }

    func updateUIViewController(_ uiViewController: MPMediaPickerController, context: Context) {
        // No dynamic updates required.
    }

    // MARK: - Coordinator

    final class Coordinator: NSObject, MPMediaPickerControllerDelegate {

        private let onSongSelected: (String, String, String, TimeInterval) -> Void
        private let onCancel: () -> Void

        init(
            onSongSelected: @escaping (String, String, String, TimeInterval) -> Void,
            onCancel: @escaping () -> Void
        ) {
            self.onSongSelected = onSongSelected
            self.onCancel = onCancel
        }

        // MARK: MPMediaPickerControllerDelegate

        func mediaPicker(
            _ mediaPicker: MPMediaPickerController,
            didPickMediaItems mediaItemCollection: MPMediaItemCollection
        ) {
            guard let item = mediaItemCollection.items.first else {
                mediaPicker.dismiss(animated: true)
                onCancel()
                return
            }

            let artist   = item.artist   ?? ""
            let title    = item.title    ?? ""
            let album    = item.albumTitle ?? ""
            let duration = item.playbackDuration

            mediaPicker.dismiss(animated: true) { [weak self] in
                self?.onSongSelected(artist, title, album, duration)
            }
        }

        func mediaPickerDidCancel(_ mediaPicker: MPMediaPickerController) {
            mediaPicker.dismiss(animated: true) { [weak self] in
                self?.onCancel()
            }
        }
    }
}

// MARK: - View+musicPicker modifier

extension View {
    /// Presents `MusicPicker` as a full-screen cover when `isPresented` is `true`.
    ///
    /// ```swift
    /// .musicPicker(isPresented: $showPicker) { artist, title, album, duration in
    ///     handleSelection(artist, title, album, duration)
    /// } onCancel: {
    ///     showPicker = false
    /// }
    /// ```
    func musicPicker(
        isPresented: Binding<Bool>,
        onSongSelected: @escaping (String, String, String, TimeInterval) -> Void,
        onCancel: @escaping () -> Void
    ) -> some View {
        self.fullScreenCover(isPresented: isPresented) {
            MusicPicker(
                onSongSelected: { artist, title, album, duration in
                    isPresented.wrappedValue = false
                    onSongSelected(artist, title, album, duration)
                },
                onCancel: {
                    isPresented.wrappedValue = false
                    onCancel()
                }
            )
            // Transparent background so the system picker chrome shows through.
            .background(Color(hex: "0E0E10").ignoresSafeArea())
        }
    }
}

// MARK: - UIColor hex convenience (local extension)

private extension UIColor {
    /// Initialises a UIColor from a six-digit hex string (without leading `#`).
    convenience init(hex: String) {
        var hexSanitised = hex.trimmingCharacters(in: .whitespacesAndNewlines)
        hexSanitised = hexSanitised.hasPrefix("#") ? String(hexSanitised.dropFirst()) : hexSanitised

        var rgb: UInt64 = 0
        Scanner(string: hexSanitised).scanHexInt64(&rgb)

        let r = CGFloat((rgb & 0xFF0000) >> 16) / 255.0
        let g = CGFloat((rgb & 0x00FF00) >>  8) / 255.0
        let b = CGFloat( rgb & 0x0000FF        ) / 255.0

        self.init(red: r, green: g, blue: b, alpha: 1.0)
    }
}
