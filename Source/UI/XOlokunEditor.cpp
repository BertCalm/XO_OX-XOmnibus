// XOlokunEditor.cpp — Implementation file for XOlokunEditor.
//
// Separating createEditor() into its own translation unit keeps the
// heavyweight UI includes (ExportDialog, CouplingVisualizer, PlaySurface…)
// out of XOlokunProcessor.cpp's compilation unit. This avoids circular-include
// issues between the editor's GalleryColors namespace block and the headers
// that both include XOlokunEditor.h and depend on GalleryColors.

#include "XOlokunEditor.h"
#include "../XOlokunProcessor.h"

namespace xolokun {

juce::AudioProcessorEditor* XOlokunProcessor::createEditor()
{
    return new XOlokunEditor(*this);
}

} // namespace xolokun
