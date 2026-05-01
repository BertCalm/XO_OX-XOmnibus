// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
// Wave 5 D3: XOuija pin / capture / per-engine routing model.
//
// XouijaPinStore owns:
//   - A live "pin" — the current XOuija (circleX, influenceY) position marked as
//     a ModSource.  While pinned, the values are frozen at the moment of pinning
//     and exposed to ModRoutingModel as ModSourceId::XouijaCell.
//   - 4 named capture slots — snapshots the user can recall later.
//   - A per-slot engine target (Global / Slot0…Slot3) — routes a captured or live
//     pin to a specific engine slot rather than applying globally.
//
// Coordination notes:
//   - D1 (cell layers) reads XouijaPinStore to decorate cell badge overlays.
//   - D2 (mood) reads targetSlot to determine per-engine mood filter.
//   - C5 (SlotModSourceRegistry, #1360 SHIPPED): XouijaPinStore::onPinChanged is
//     wired in PlaySurface::setProcessor() to push bipolar X+Y values into
//     SlotModSourceRegistry → processBlock reads them as ModSourceId::XouijaX /
//     ModSourceId::XouijaY.  See #1383 A4 for the reconciled wiring.
//   - ModRoutingModel: call pinStore.registerWithModRouter(modModel) to create a
//     live ModSourceId::XouijaX / XouijaY route entry that tracks the pinned values.
//
// Thread safety: all methods are message-thread-only (same as ModRoutingModel).
//
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>
#include <functional>
#include "../../Future/UI/ModRouting/ModSourceHandle.h" // ModSourceId

namespace xoceanus
{

//==============================================================================
// XouijaCaptureSlot — a named, persistent snapshot of a XOuija (X, Y) position
// plus a per-engine routing target.
//
struct XouijaCaptureSlot
{
    // User-visible name for this capture slot (editable via context menu).
    juce::String name;

    // Normalized [0, 1] position captured from XOuijaPanel.
    float circleX = 0.5f;
    float influenceY = 0.0f;

    // Whether this slot contains a valid snapshot (vs. empty/default).
    bool hasCapture = false;

    //==========================================================================
    // Per-engine routing target.
    //
    // Global  — modulation applies to all engine slots simultaneously (default).
    // Slot0-3 — modulation applies only to the selected engine slot.
    //
    // The PerEnginePatternSequencer (Wave5-C1) uses the same slot numbering.
    // When B3 per-engine chord/seq routing is active, set targetSlot to match.
    //
    enum class EngineTarget : uint8_t
    {
        Global = 0,
        Slot0 = 1,
        Slot1 = 2,
        Slot2 = 3,
        Slot3 = 4,
    };
    EngineTarget targetSlot = EngineTarget::Global;

    //==========================================================================
    // Serialisation

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree vt("XouijaCaptureSlot");
        vt.setProperty("name", name, nullptr);
        vt.setProperty("circleX", static_cast<double>(circleX), nullptr);
        vt.setProperty("influenceY", static_cast<double>(influenceY), nullptr);
        vt.setProperty("hasCapture", hasCapture, nullptr);
        vt.setProperty("targetSlot", static_cast<int>(targetSlot), nullptr);
        return vt;
    }

    void fromValueTree(const juce::ValueTree& vt)
    {
        if (!vt.isValid() || !vt.hasType("XouijaCaptureSlot"))
            return;
        name = vt.getProperty("name", "").toString();
        circleX = juce::jlimit(0.0f, 1.0f, static_cast<float>(static_cast<double>(vt.getProperty("circleX", 0.5))));
        influenceY =
            juce::jlimit(0.0f, 1.0f, static_cast<float>(static_cast<double>(vt.getProperty("influenceY", 0.0))));
        hasCapture = static_cast<bool>(vt.getProperty("hasCapture", false));
        const int t = static_cast<int>(vt.getProperty("targetSlot", 0));
        targetSlot = (t >= 0 && t <= 4) ? static_cast<EngineTarget>(t) : EngineTarget::Global;
    }

    static juce::String engineTargetName(EngineTarget t)
    {
        switch (t)
        {
        case EngineTarget::Global:
            return "Global";
        case EngineTarget::Slot0:
            return "Slot 0";
        case EngineTarget::Slot1:
            return "Slot 1";
        case EngineTarget::Slot2:
            return "Slot 2";
        case EngineTarget::Slot3:
            return "Slot 3";
        default:
            return "Global";
        }
    }
};

//==============================================================================
// XouijaPinStore — live pin + 4 named capture slots + per-engine routing.
//
// Usage in XOuijaPanel:
//
//   // Construction (message thread):
//   XouijaPinStore pinStore_;
//
//   // Right-click → "Pin as ModSource":
//   pinStore_.pin(circleX_, influenceY_);
//
//   // Right-click → "Capture to Slot N":
//   pinStore_.captureToSlot(n, circleX_, influenceY_, "My Capture");
//
//   // Right-click → "Recall Slot N":
//   auto& slot = pinStore_.getSlot(n);
//   if (slot.hasCapture) { circleX_ = slot.circleX; influenceY_ = slot.influenceY; }
//
//   // Right-click → "Route to Slot N":
//   pinStore_.setTargetSlot(slotIdx, XouijaCaptureSlot::EngineTarget::Slot2);
//
//   // Read live pin for DSP polling (until C5 lands):
//   float x = pinStore_.getPinnedCircleX();
//   float y = pinStore_.getPinnedInfluenceY();
//
class XouijaPinStore
{
public:
    static constexpr int kNumCaptureSlots = 4;

    // Explicit default constructor required: JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR
    // deletes copy ctor which can suppress implicit default ctor on strict clang builds.
    XouijaPinStore() = default;

    //==========================================================================
    // Live pin

    // Pin the current XOuija position as a ModSource.  Fires onPinChanged.
    void pin(float circleX, float influenceY)
    {
        isPinned_ = true;
        pinnedCircleX_ = juce::jlimit(0.0f, 1.0f, circleX);
        pinnedInfluenceY_ = juce::jlimit(0.0f, 1.0f, influenceY);
        notifyListeners();
    }

    // Remove the live pin.  Fires onPinChanged.
    void unpin()
    {
        if (!isPinned_)
            return;
        isPinned_ = false;
        notifyListeners();
    }

    bool hasPinnedValue() const noexcept { return isPinned_; }

    // Bipolar [-1, +1] version of the pinned circle position.
    // Returns 0 when not pinned.
    float getPinnedCircleX() const noexcept { return isPinned_ ? (pinnedCircleX_ * 2.0f - 1.0f) : 0.0f; }
    float getPinnedInfluenceY() const noexcept { return isPinned_ ? (pinnedInfluenceY_ * 2.0f - 1.0f) : 0.0f; }

    // Raw [0,1] versions (for recalling to the panel position).
    float getRawPinnedCircleX() const noexcept { return pinnedCircleX_; }
    float getRawPinnedInfluenceY() const noexcept { return pinnedInfluenceY_; }

    //==========================================================================
    // Capture slots

    // Snapshot the given position into slot n (0-3).
    // Silently clamps n to [0, kNumCaptureSlots-1].
    void captureToSlot(int n, float circleX, float influenceY, const juce::String& slotName = {})
    {
        if (n < 0 || n >= kNumCaptureSlots)
            return;
        auto& s = slots_[static_cast<std::size_t>(n)];
        s.circleX = juce::jlimit(0.0f, 1.0f, circleX);
        s.influenceY = juce::jlimit(0.0f, 1.0f, influenceY);
        s.hasCapture = true;
        if (slotName.isNotEmpty())
            s.name = slotName;
        else if (s.name.isEmpty())
            s.name = "Slot " + juce::String(n + 1);
        notifyListeners();
    }

    // Clear a capture slot.
    void clearSlot(int n)
    {
        if (n < 0 || n >= kNumCaptureSlots)
            return;
        slots_[static_cast<std::size_t>(n)] = {};
        notifyListeners();
    }

    const XouijaCaptureSlot& getSlot(int n) const noexcept
    {
        return slots_[static_cast<std::size_t>(juce::jlimit(0, kNumCaptureSlots - 1, n))];
    }

    //==========================================================================
    // Per-engine routing target

    // Set the engine routing target for capture slot n.
    void setTargetSlot(int n, XouijaCaptureSlot::EngineTarget target)
    {
        if (n < 0 || n >= kNumCaptureSlots)
            return;
        slots_[static_cast<std::size_t>(n)].targetSlot = target;
        notifyListeners();
    }

    // Set the engine routing target for the live pin.
    void setPinTargetSlot(XouijaCaptureSlot::EngineTarget target)
    {
        pinTargetSlot_ = target;
        notifyListeners();
    }

    XouijaCaptureSlot::EngineTarget getPinTargetSlot() const noexcept { return pinTargetSlot_; }

    //==========================================================================
    // Change notifications — UI registers to redraw badges when store changes.

    void addListener(juce::ChangeListener* l) { broadcaster_.addChangeListener(l); }
    void removeListener(juce::ChangeListener* l) { broadcaster_.removeChangeListener(l); }

    //==========================================================================
    // Callback for SlotModSourceRegistry bridge (#1383 A4).
    //
    // Fires whenever the pinned value changes (pin / unpin / position update).
    // When pinned, bx/by carry the bipolar [-1,+1] planchette position.
    // When unpinned, bx=0/by=0 so the registry resets to neutral.
    //
    // Wired in PlaySurface::setProcessor():
    //
    //   xouijaPanel_.getPinStore().onPinChanged =
    //       [this](float bx, float by) {
    //           auto& reg = processor_->getModSourceRegistry();
    //           reg.updateSourceValue(ModSourceId::XouijaX, bx);
    //           reg.updateSourceValue(ModSourceId::XouijaY, by);
    //           reg.updateSourceValue(ModSourceId::XouijaCell, bx, by);
    //       };
    //
    // bx / by are bipolar [-1, +1].  Message-thread only.
    //
    std::function<void(float /*bx*/, float /*by*/)> onPinChanged;

    //==========================================================================
    // Serialisation

    juce::ValueTree toValueTree() const
    {
        juce::ValueTree vt("XouijaPinStore");
        vt.setProperty("isPinned", isPinned_, nullptr);
        vt.setProperty("pinnedCircleX", static_cast<double>(pinnedCircleX_), nullptr);
        vt.setProperty("pinnedInfluenceY", static_cast<double>(pinnedInfluenceY_), nullptr);
        vt.setProperty("pinTargetSlot", static_cast<int>(pinTargetSlot_), nullptr);
        for (int i = 0; i < kNumCaptureSlots; ++i)
            vt.appendChild(slots_[static_cast<std::size_t>(i)].toValueTree(), nullptr);
        return vt;
    }

    void fromValueTree(const juce::ValueTree& vt)
    {
        if (!vt.isValid() || !vt.hasType("XouijaPinStore"))
            return;
        isPinned_ = static_cast<bool>(vt.getProperty("isPinned", false));
        pinnedCircleX_ =
            juce::jlimit(0.0f, 1.0f, static_cast<float>(static_cast<double>(vt.getProperty("pinnedCircleX", 0.5))));
        pinnedInfluenceY_ =
            juce::jlimit(0.0f, 1.0f, static_cast<float>(static_cast<double>(vt.getProperty("pinnedInfluenceY", 0.0))));
        const int t = static_cast<int>(vt.getProperty("pinTargetSlot", 0));
        pinTargetSlot_ = (t >= 0 && t <= 4) ? static_cast<XouijaCaptureSlot::EngineTarget>(t)
                                            : XouijaCaptureSlot::EngineTarget::Global;
        int slotIdx = 0;
        for (int c = 0; c < vt.getNumChildren() && slotIdx < kNumCaptureSlots; ++c)
        {
            auto child = vt.getChild(c);
            if (child.hasType("XouijaCaptureSlot"))
            {
                slots_[static_cast<std::size_t>(slotIdx)].fromValueTree(child);
                ++slotIdx;
            }
        }
        notifyListeners();
    }

private:
    void notifyListeners()
    {
        broadcaster_.sendChangeMessage();
        if (onPinChanged)
        {
            if (isPinned_)
                onPinChanged(getPinnedCircleX(), getPinnedInfluenceY());
            else
                onPinChanged(0.0f, 0.0f);
        }
    }

    // Live pin state
    bool isPinned_ = false;
    float pinnedCircleX_ = 0.5f;
    float pinnedInfluenceY_ = 0.0f;
    XouijaCaptureSlot::EngineTarget pinTargetSlot_ = XouijaCaptureSlot::EngineTarget::Global;

    // Named capture slots
    std::array<XouijaCaptureSlot, kNumCaptureSlots> slots_;

    juce::ChangeBroadcaster broadcaster_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(XouijaPinStore)
};

//==============================================================================
// XouijaPinContextMenu — right-click menu builder for XOuijaPanel.
//
// Builds the popup menu items for pin / capture / routing actions.
// Call showFor() from XOuijaPanel::mouseUp (right-click).
//
// Menu structure:
//   ─── Pin / Unpin ───────────────────────────────────────────────────
//   [x] Pin as ModSource  (check = currently pinned)
//       Route pin to:  ▶  Global / Slot 0 / Slot 1 / Slot 2 / Slot 3
//   ─── Capture ───────────────────────────────────────────────────────
//   Capture to Slot 1  "name if any"
//   Capture to Slot 2  "name if any"
//   Capture to Slot 3  "name if any"
//   Capture to Slot 4  "name if any"
//   ─── Recall ────────────────────────────────────────────────────────
//   Recall Slot 1  "name"  (greyed if empty)
//   Recall Slot 2  "name"  ...
//   Recall Slot 3  "name"
//   Recall Slot 4  "name"
//   ─── Clear ─────────────────────────────────────────────────────────
//   Clear Slot N  (shown only for occupied slots)
//
struct XouijaPinContextMenu
{
    // Item ID ranges (non-overlapping):
    //   1        = toggle pin
    //  10-14     = set pin target (10=Global, 11=Slot0..14=Slot3)
    //  20-23     = capture to slot 0-3
    //  30-33     = recall slot 0-3
    //  40-43     = set target for slot 0-3 (submenu: 40 = slot0 target menu trigger)
    //  50-99     = inner sub-items for per-slot target (50+slotIdx*5 + targetEnum)
    //  100-103   = clear slot 0-3
    //
    static constexpr int kTogglePin = 1;
    static constexpr int kPinTargetBase = 10; // +0=Global +1=Slot0 +2=Slot1 +3=Slot2 +4=Slot3
    static constexpr int kCaptureBase = 20;   // +slotIdx
    static constexpr int kRecallBase = 30;    // +slotIdx
    static constexpr int kSlotTgtBase = 50;   // +slotIdx*5 + targetEnum
    static constexpr int kClearBase = 100;    // +slotIdx

    static void show(XouijaPinStore& store, float curCircleX, float curInfluenceY, juce::Component* relativeTo,
                     std::function<void(float x, float y)> onRecall)
    {
        juce::PopupMenu menu;

        // ── Pin / Unpin ────────────────────────────────────────────────────
        menu.addSectionHeader("XOuija Pin");
        const bool pinned = store.hasPinnedValue();
        menu.addItem(kTogglePin, pinned ? "Unpin ModSource" : "Pin as ModSource",
                     /* enabled = */ true, /* checked = */ pinned);

        // Route pin to engine submenu
        if (pinned)
        {
            juce::PopupMenu pinTargetMenu;
            for (int t = 0; t <= 4; ++t)
            {
                auto et = static_cast<XouijaCaptureSlot::EngineTarget>(t);
                bool active = (store.getPinTargetSlot() == et);
                pinTargetMenu.addItem(kPinTargetBase + t, XouijaCaptureSlot::engineTargetName(et), true, active);
            }
            menu.addSubMenu("Route pin to engine...", pinTargetMenu);
        }

        menu.addSeparator();

        // ── Capture ────────────────────────────────────────────────────────
        menu.addSectionHeader("Capture");
        for (int i = 0; i < XouijaPinStore::kNumCaptureSlots; ++i)
        {
            const auto& s = store.getSlot(i);
            juce::String label = "Capture to Slot " + juce::String(i + 1);
            if (s.hasCapture && s.name.isNotEmpty())
                label += "  (" + s.name + ")";
            menu.addItem(kCaptureBase + i, label);
        }

        menu.addSeparator();

        // ── Recall ─────────────────────────────────────────────────────────
        menu.addSectionHeader("Recall");
        for (int i = 0; i < XouijaPinStore::kNumCaptureSlots; ++i)
        {
            const auto& s = store.getSlot(i);
            juce::String label = "Recall Slot " + juce::String(i + 1);
            if (s.hasCapture)
            {
                if (s.name.isNotEmpty())
                    label += "  \"" + s.name + "\"";
                label += "  [X:" + juce::String(s.circleX, 2) + " Y:" + juce::String(s.influenceY, 2) + "]";
            }

            // Recall submenu for occupied slots includes "Route to..."
            if (s.hasCapture)
            {
                juce::PopupMenu recallSub;
                recallSub.addItem(kRecallBase + i, "Recall position");
                recallSub.addSeparator();
                juce::PopupMenu tgtSub;
                for (int t = 0; t <= 4; ++t)
                {
                    auto et = static_cast<XouijaCaptureSlot::EngineTarget>(t);
                    bool active = (s.targetSlot == et);
                    tgtSub.addItem(kSlotTgtBase + i * 5 + t, XouijaCaptureSlot::engineTargetName(et), true, active);
                }
                recallSub.addSubMenu("Route to engine...", tgtSub);
                recallSub.addItem(kClearBase + i, "Clear slot");
                menu.addSubMenu(label, recallSub);
            }
            else
            {
                menu.addItem(kRecallBase + i, label + "  (empty)", false);
            }
        }

        menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(relativeTo),
                           [&store, curCircleX, curInfluenceY, onRecall](int result)
                           {
                               if (result <= 0)
                                   return;

                               if (result == kTogglePin)
                               {
                                   if (store.hasPinnedValue())
                                       store.unpin();
                                   else
                                       store.pin(curCircleX, curInfluenceY);
                                   return;
                               }

                               if (result >= kPinTargetBase && result < kPinTargetBase + 5)
                               {
                                   auto et = static_cast<XouijaCaptureSlot::EngineTarget>(result - kPinTargetBase);
                                   store.setPinTargetSlot(et);
                                   return;
                               }

                               if (result >= kCaptureBase && result < kCaptureBase + XouijaPinStore::kNumCaptureSlots)
                               {
                                   int idx = result - kCaptureBase;
                                   store.captureToSlot(idx, curCircleX, curInfluenceY);
                                   return;
                               }

                               if (result >= kRecallBase && result < kRecallBase + XouijaPinStore::kNumCaptureSlots)
                               {
                                   int idx = result - kRecallBase;
                                   const auto& s = store.getSlot(idx);
                                   if (s.hasCapture && onRecall)
                                       onRecall(s.circleX, s.influenceY);
                                   return;
                               }

                               if (result >= kSlotTgtBase &&
                                   result < kSlotTgtBase + XouijaPinStore::kNumCaptureSlots * 5)
                               {
                                   int raw = result - kSlotTgtBase;
                                   int si = raw / 5;
                                   int ti = raw % 5;
                                   store.setTargetSlot(si, static_cast<XouijaCaptureSlot::EngineTarget>(ti));
                                   return;
                               }

                               if (result >= kClearBase && result < kClearBase + XouijaPinStore::kNumCaptureSlots)
                               {
                                   store.clearSlot(result - kClearBase);
                                   return;
                               }
                           });
    }
};

} // namespace xoceanus
