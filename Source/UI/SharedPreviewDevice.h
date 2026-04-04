// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_audio_devices/juce_audio_devices.h>

namespace xoceanus
{

/// Shared audio device for all in-process preview players.
///
/// Multiple components (OutshinePreviewPlayer, ExportDialog, etc.) that want to
/// play audio previews must share a single AudioDeviceManager; if each owns one,
/// only the first to claim the hardware device succeeds — the rest silently fail.
///
/// Usage:
///   juce::SharedResourcePointer<SharedPreviewDevice> sharedDevice;
///   sharedDevice->manager.addAudioCallback(...);
///
/// The SharedResourcePointer reference-counts the instance: it is created on
/// first use and destroyed when the last holder is destroyed.
struct SharedPreviewDevice
{
    juce::AudioDeviceManager manager;

    SharedPreviewDevice()
    {
        manager.initialiseWithDefaultDevices(0, 2);
    }
};

} // namespace xoceanus
