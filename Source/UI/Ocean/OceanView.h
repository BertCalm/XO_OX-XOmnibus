// Other existing content

/** Fired when the user triggers Undo (Cmd+Z). Wire to processor.getUndoManager().undo(). */
std::function<void()> onUndoRequested;

/** Fired when the user triggers Redo (Cmd+Shift+Z). Wire to processor.getUndoManager().redo(). */
std::function<void()> onRedoRequested;

//==========================================================================
// The rest of the file content that follows...