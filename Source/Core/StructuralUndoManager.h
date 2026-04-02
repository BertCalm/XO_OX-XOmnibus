// SPDX-License-Identifier: MIT
// Copyright (c) 2026 XO_OX Designs
#pragma once
#include <juce_core/juce_core.h>
#include "SynthEngine.h"   // for CouplingType
#include <vector>
#include <deque>
#include <cassert>

namespace xoceanus {

//==============================================================================
// StructuralUndoManager — Undo/redo for non-APVTS state changes.
//
// JUCE's APVTS handles parameter undo automatically via its own UndoManager.
// Structural changes — which engine occupies which slot, which coupling routes
// are active, which preset is loaded — live outside APVTS and have no native
// undo. This class fills that gap.
//
// Design principles
// -----------------
//  • Command pattern: every structural mutation is recorded as a reversible
//    StructuralCommand with both "undo" and "redo" payloads baked in at record
//    time.
//  • Transaction model: related mutations (e.g. engine swap + coupling reload)
//    are grouped by beginTransaction() / endTransaction() into one undo step.
//    Open transactions accumulate commands; closing commits them atomically.
//  • Pure / passive: the manager does NOT execute undo or redo itself. It
//    returns the commands that the caller (XOceanusProcessor) must apply to
//    EngineRegistry and MegaCouplingMatrix. This keeps the manager testable in
//    isolation and free of dependency on the processor or audio thread.
//  • Circular history: a fixed-capacity deque (default 50 steps) prevents
//    unbounded memory growth. Oldest entry is evicted when the limit is hit.
//  • Thread safety: all methods must be called on the message thread only.
//    Structural changes do not happen on the audio thread so no lock is needed.
//    A juce::MessageManager::Lock assertion guards debug builds.
//  • Redo invalidation: any new recording while there are redo steps collapses
//    the redo stack (standard DAW behaviour).
//
// Usage example (in XOceanusProcessor)
// --------------------------------------
//   structuralUndo.beginTransaction ("Swap slot 0 to Overdub");
//   structuralUndo.recordEngineSwap (0, "OddfeliX", "Overdub");
//   structuralUndo.recordPresetLoad (0, "Current preset", "Init");
//   structuralUndo.endTransaction();
//
//   // Later, on Cmd+Z:
//   auto cmds = structuralUndo.undo();
//   for (auto& cmd : cmds) applyStructuralCommand (cmd);
//
// The applyStructuralCommand() helper on the processor interprets each command's
// `type` and uses the `undo*` fields to revert the change.
//
class StructuralUndoManager {
public:
    //==========================================================================
    // StructuralCommand — atomic unit of reversible structural change.
    //
    // Each field pair follows the convention:
    //   record*  — the value that was in place BEFORE the change (used for undo)
    //   apply*   — the value that was put in place BY the change (used for redo)
    //
    // The caller must inspect `type` to know which fields are populated.
    //
    struct StructuralCommand {
        enum class Type {
            EngineSwap,        // A slot's engine type changed
            CouplingAdd,       // A coupling route was added
            CouplingRemove,    // A coupling route was removed
            CouplingModify,    // A coupling route's amount was changed
            PresetLoad         // A preset was loaded into a slot
        };

        Type type;

        // --- EngineSwap fields ---
        int            slot          = -1;
        juce::String   oldEngine;   // engine that was in slot before swap  (undo → put this back)
        juce::String   newEngine;   // engine that was put in slot by swap  (redo → put this back)

        // --- CouplingAdd / CouplingRemove / CouplingModify fields ---
        int            sourceSlot   = -1;
        int            destSlot     = -1;
        CouplingType   couplingType = CouplingType::AmpToFilter;
        float          oldAmount    = 0.0f;  // amount before (undo target)
        float          newAmount    = 0.0f;  // amount after  (redo target)

        // --- PresetLoad fields ---
        // slot reuses the slot field above
        juce::String   oldPreset;   // preset name/ID that was active before load (undo)
        juce::String   newPreset;   // preset name/ID that was loaded            (redo)
    };

    //==========================================================================
    // Transaction — a named group of StructuralCommands that undo as one step.
    //
    struct Transaction {
        juce::String                     description;
        std::vector<StructuralCommand>   commands;
    };

    //==========================================================================
    // Construction

    StructuralUndoManager() = default;

    // Non-copyable, non-moveable — singleton-style ownership by the processor.
    StructuralUndoManager(const StructuralUndoManager&)            = delete;
    StructuralUndoManager& operator=(const StructuralUndoManager&) = delete;
    StructuralUndoManager(StructuralUndoManager&&)                 = delete;
    StructuralUndoManager& operator=(StructuralUndoManager&&)      = delete;

    //==========================================================================
    // Transaction control

    // Open a new transaction. All record*() calls between here and
    // endTransaction() belong to this step. Nesting is not supported; calling
    // beginTransaction() while one is already open triggers an assertion in
    // debug builds and silently closes the previous one in release builds.
    void beginTransaction(const juce::String& description)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(!transactionOpen);  // Mismatched begin/end — fix the caller

        if (transactionOpen)
            endTransaction();       // release safety valve

        currentTransaction.description = description;
        currentTransaction.commands.clear();
        transactionOpen = true;
    }

    // Close the current transaction and push it onto the undo stack.
    // If no commands were recorded (empty transaction), nothing is pushed.
    // Any redo steps that existed are invalidated.
    void endTransaction()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(transactionOpen);   // Mismatched begin/end — fix the caller

        if (!transactionOpen)
            return;

        transactionOpen = false;

        if (currentTransaction.commands.empty())
            return;   // nothing happened — don't pollute the undo stack

        // Invalidate redo stack (standard behaviour: new action cancels redo)
        redoStack.clear();

        // Push onto undo stack
        undoStack.push_back(std::move(currentTransaction));

        // Enforce history limit — evict oldest entry
        while (static_cast<int>(undoStack.size()) > maxHistory)
            undoStack.pop_front();

        currentTransaction = {};
    }

    //==========================================================================
    // Recording (call between beginTransaction / endTransaction)

    // Record that slot `slot` changed from engine `oldEngine` to `newEngine`.
    void recordEngineSwap(int slot,
                          const juce::String& oldEngine,
                          const juce::String& newEngine)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(transactionOpen);

        StructuralCommand cmd;
        cmd.type      = StructuralCommand::Type::EngineSwap;
        cmd.slot      = slot;
        cmd.oldEngine = oldEngine;
        cmd.newEngine = newEngine;
        appendCommand(std::move(cmd));
    }

    // Record that a new coupling route was added.
    // Undo removes it; redo re-adds it at `amount`.
    void recordCouplingAdd(int sourceSlot, int destSlot,
                           CouplingType couplingType, float amount)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(transactionOpen);

        StructuralCommand cmd;
        cmd.type        = StructuralCommand::Type::CouplingAdd;
        cmd.sourceSlot  = sourceSlot;
        cmd.destSlot    = destSlot;
        cmd.couplingType = couplingType;
        cmd.oldAmount   = 0.0f;    // did not exist before
        cmd.newAmount   = amount;
        appendCommand(std::move(cmd));
    }

    // Record that an existing coupling route was removed.
    // Undo re-adds it at `amount`; redo removes it again.
    void recordCouplingRemove(int sourceSlot, int destSlot,
                              CouplingType couplingType, float amount)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(transactionOpen);

        StructuralCommand cmd;
        cmd.type        = StructuralCommand::Type::CouplingRemove;
        cmd.sourceSlot  = sourceSlot;
        cmd.destSlot    = destSlot;
        cmd.couplingType = couplingType;
        cmd.oldAmount   = amount;  // was at this amount before removal
        cmd.newAmount   = 0.0f;   // now gone
        appendCommand(std::move(cmd));
    }

    // Record that an existing coupling route's amount changed.
    // Undo restores `oldAmount`; redo restores `newAmount`.
    void recordCouplingModify(int sourceSlot, int destSlot,
                              CouplingType couplingType,
                              float oldAmount, float newAmount)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(transactionOpen);

        StructuralCommand cmd;
        cmd.type         = StructuralCommand::Type::CouplingModify;
        cmd.sourceSlot   = sourceSlot;
        cmd.destSlot     = destSlot;
        cmd.couplingType = couplingType;
        cmd.oldAmount    = oldAmount;
        cmd.newAmount    = newAmount;
        appendCommand(std::move(cmd));
    }

    // Record that slot `slot` loaded a new preset.
    // `oldPreset` is the name/ID that was active before; `newPreset` is what
    // was loaded. The processor is responsible for keeping track of the current
    // preset name so it can pass it here at record time.
    void recordPresetLoad(int slot,
                          const juce::String& oldPreset,
                          const juce::String& newPreset)
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        jassert(transactionOpen);

        StructuralCommand cmd;
        cmd.type      = StructuralCommand::Type::PresetLoad;
        cmd.slot      = slot;
        cmd.oldPreset = oldPreset;
        cmd.newPreset = newPreset;
        appendCommand(std::move(cmd));
    }

    //==========================================================================
    // Query

    bool canUndo() const noexcept { return !undoStack.empty(); }
    bool canRedo() const noexcept { return !redoStack.empty(); }

    // Human-readable description of the next undo step (for menu items).
    // Returns empty string if canUndo() is false.
    juce::String getUndoDescription() const
    {
        if (undoStack.empty()) return {};
        return undoStack.back().description;
    }

    // Human-readable description of the next redo step.
    // Returns empty string if canRedo() is false.
    juce::String getRedoDescription() const
    {
        if (redoStack.empty()) return {};
        return redoStack.back().description;
    }

    // Number of steps available to undo.
    int getUndoDepth()  const noexcept { return static_cast<int>(undoStack.size()); }

    // Number of steps available to redo.
    int getRedoDepth()  const noexcept { return static_cast<int>(redoStack.size()); }

    int  getMaxHistory() const noexcept { return maxHistory; }
    void setMaxHistory(int max) noexcept
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        maxHistory = juce::jmax(1, max);
        // Trim stacks to new limit
        while (static_cast<int>(undoStack.size()) > maxHistory)
            undoStack.pop_front();
        while (static_cast<int>(redoStack.size()) > maxHistory)
            redoStack.pop_front();
    }

    //==========================================================================
    // Undo / Redo
    //
    // Both methods return the list of commands the caller must apply to revert
    // (undo) or reapply (redo) the structural change. The commands are ordered
    // so that the caller may apply them in the returned vector order.
    //
    // For undo: commands are returned in reverse-record order (last recorded
    // command is first to undo), so that interleaved changes within a
    // transaction are rolled back in the correct dependency order.
    //
    // For redo: commands are returned in original record order.

    // Pop the top undo step and return commands to revert it.
    // Returns an empty vector if canUndo() is false.
    // The caller MUST apply all returned commands before the UI is updated.
    std::vector<StructuralCommand> undo()
    {
        JUCE_ASSERT_MESSAGE_THREAD;

        if (undoStack.empty())
            return {};

        // Any open transaction should have been closed before undo is called.
        // Silently close it to avoid corrupted state.
        if (transactionOpen)
        {
            jassertfalse;
            transactionOpen = false;
            currentTransaction = {};
        }

        Transaction step = std::move(undoStack.back());
        undoStack.pop_back();

        // Build reversed-command list for undo execution.
        // Each command is "inverted": undo means applying the old state.
        std::vector<StructuralCommand> undoCmds = buildUndoCommands(step);

        // Push the step (with original commands) onto redo stack so redo can
        // reapply the new state.
        redoStack.push_back(std::move(step));
        while (static_cast<int>(redoStack.size()) > maxHistory)
            redoStack.pop_front();

        return undoCmds;
    }

    // Pop the top redo step and return commands to reapply it.
    // Returns an empty vector if canRedo() is false.
    std::vector<StructuralCommand> redo()
    {
        JUCE_ASSERT_MESSAGE_THREAD;

        if (redoStack.empty())
            return {};

        if (transactionOpen)
        {
            jassertfalse;
            transactionOpen = false;
            currentTransaction = {};
        }

        Transaction step = std::move(redoStack.back());
        redoStack.pop_back();

        // For redo we reapply the commands in original order using "new" state.
        std::vector<StructuralCommand> redoCmds = buildRedoCommands(step);

        // Push back onto undo stack so the user can undo this redo.
        undoStack.push_back(std::move(step));
        while (static_cast<int>(undoStack.size()) > maxHistory)
            undoStack.pop_front();

        return redoCmds;
    }

    //==========================================================================
    // Maintenance

    // Clear both stacks. Call on full state reset (e.g. new project load).
    void clear()
    {
        JUCE_ASSERT_MESSAGE_THREAD;
        undoStack.clear();
        redoStack.clear();
        currentTransaction = {};
        transactionOpen    = false;
    }

    // Returns true if a transaction is currently open. Useful for asserts in
    // the processor to verify record calls are made inside transactions.
    bool isTransactionOpen() const noexcept { return transactionOpen; }

private:
    //==========================================================================
    // Implementation helpers

    void appendCommand(StructuralCommand cmd)
    {
        if (!transactionOpen)
        {
            // Guard: silently wrap in an anonymous transaction so state remains
            // consistent even if the caller forgot beginTransaction().
            jassertfalse;
            currentTransaction.description = "Structural change";
            transactionOpen = true;
        }
        currentTransaction.commands.push_back(std::move(cmd));
    }

    // Build the list of commands the caller should execute to UNDO a transaction.
    // Commands are reversed so that within a transaction the last-recorded
    // change is the first to be reverted (correct dependency order).
    // Each command is transformed so its "new" field points at the old state.
    static std::vector<StructuralCommand> buildUndoCommands(const Transaction& step)
    {
        std::vector<StructuralCommand> result;
        result.reserve(step.commands.size());

        // Iterate in reverse (last recorded = first to undo)
        for (int i = static_cast<int>(step.commands.size()) - 1; i >= 0; --i)
        {
            const auto& src = step.commands[static_cast<size_t>(i)];
            StructuralCommand inv;
            inv.type         = invertedType(src.type);
            inv.slot         = src.slot;
            inv.sourceSlot   = src.sourceSlot;
            inv.destSlot     = src.destSlot;
            inv.couplingType = src.couplingType;

            // Swap old/new so the caller restores the previous state
            inv.oldEngine    = src.newEngine;
            inv.newEngine    = src.oldEngine;
            inv.oldPreset    = src.newPreset;
            inv.newPreset    = src.oldPreset;
            inv.oldAmount    = src.newAmount;
            inv.newAmount    = src.oldAmount;

            result.push_back(std::move(inv));
        }

        return result;
    }

    // Build the list of commands the caller should execute to REDO a transaction.
    // Commands are in original order; each command reflects the new-state values.
    static std::vector<StructuralCommand> buildRedoCommands(const Transaction& step)
    {
        // For redo, the commands stored in the transaction already have the
        // correct new-state in `newEngine` / `newPreset` / `newAmount`.
        // We just return them as-is (in original record order) so the caller
        // re-applies each change.
        return step.commands;
    }

    // For undo, CouplingAdd becomes CouplingRemove and vice versa.
    // EngineSwap and PresetLoad and CouplingModify are self-inverting
    // (the same Type, but old/new are swapped by the caller).
    static StructuralCommand::Type invertedType(StructuralCommand::Type t)
    {
        using T = StructuralCommand::Type;
        switch (t)
        {
            case T::CouplingAdd:    return T::CouplingRemove;
            case T::CouplingRemove: return T::CouplingAdd;
            case T::EngineSwap:     return T::EngineSwap;
            case T::CouplingModify: return T::CouplingModify;
            case T::PresetLoad:     return T::PresetLoad;
        }
        return t;  // unreachable — silence compiler warning
    }

    //==========================================================================
    // State

    std::deque<Transaction> undoStack;          // index 0 = oldest, back = next to undo
    std::deque<Transaction> redoStack;          // index 0 = oldest, back = next to redo
    Transaction             currentTransaction;
    bool                    transactionOpen = false;
    int                     maxHistory      = 50;
};

//==============================================================================
// Convenience: a helper the processor can use to dispatch StructuralCommands
// back to EngineRegistry and MegaCouplingMatrix after undo() / redo().
//
// The struct is intentionally minimal — it holds only the interface it needs
// and is designed to be filled in by the processor at call sites via lambdas.
//
// Typical processor usage:
//
//   StructuralCommandDispatcher dispatcher;
//   dispatcher.onEngineSwap  = [this](int slot, const juce::String& id) {
//       swapEngineInSlot(slot, id);
//   };
//   dispatcher.onCouplingAdd = [this](int src, int dst, CouplingType t, float a) {
//       couplingMatrix.addRoute({ src, dst, t, a, false, true });
//   };
//   // ... fill the rest ...
//
//   auto cmds = structuralUndo.undo();
//   dispatcher.dispatch(cmds);
//
struct StructuralCommandDispatcher {
    using SwapFn      = std::function<void(int slot, const juce::String& engineId)>;
    using CouplingFn  = std::function<void(int sourceSlot, int destSlot,
                                           CouplingType type, float amount)>;
    using RemoveFn    = std::function<void(int sourceSlot, int destSlot, CouplingType type)>;
    using ModifyFn    = std::function<void(int sourceSlot, int destSlot,
                                           CouplingType type, float amount)>;
    using PresetFn    = std::function<void(int slot, const juce::String& presetNameOrId)>;

    SwapFn      onEngineSwap;    // slot, newEngineId
    CouplingFn  onCouplingAdd;   // sourceSlot, destSlot, type, amount
    RemoveFn    onCouplingRemove;// sourceSlot, destSlot, type
    ModifyFn    onCouplingModify;// sourceSlot, destSlot, type, newAmount
    PresetFn    onPresetLoad;    // slot, presetNameOrId

    // Apply all commands in order. Missing handlers are silently skipped so
    // partial dispatch is safe during incremental integration.
    void dispatch(const std::vector<StructuralUndoManager::StructuralCommand>& cmds) const
    {
        using T = StructuralUndoManager::StructuralCommand::Type;
        for (const auto& cmd : cmds)
        {
            switch (cmd.type)
            {
                case T::EngineSwap:
                    if (onEngineSwap)
                        onEngineSwap(cmd.slot, cmd.newEngine);
                    break;

                case T::CouplingAdd:
                    if (onCouplingAdd)
                        onCouplingAdd(cmd.sourceSlot, cmd.destSlot,
                                      cmd.couplingType, cmd.newAmount);
                    break;

                case T::CouplingRemove:
                    if (onCouplingRemove)
                        onCouplingRemove(cmd.sourceSlot, cmd.destSlot,
                                         cmd.couplingType);
                    break;

                case T::CouplingModify:
                    if (onCouplingModify)
                        onCouplingModify(cmd.sourceSlot, cmd.destSlot,
                                         cmd.couplingType, cmd.newAmount);
                    break;

                case T::PresetLoad:
                    if (onPresetLoad)
                        onPresetLoad(cmd.slot, cmd.newPreset);
                    break;
            }
        }
    }
};

} // namespace xoceanus
