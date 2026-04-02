// XOceanusEditor.cpp — Implementation file for XOceanusEditor.
//
// Separating createEditor() into its own translation unit keeps the
// heavyweight UI includes (ExportDialog, CouplingVisualizer, PlaySurface…)
// out of XOceanusProcessor.cpp's compilation unit. This avoids circular-include
// issues between the editor's GalleryColors namespace block and the headers
// that both include XOceanusEditor.h and depend on GalleryColors.

#include "XOceanusEditor.h"
#include "../XOceanusProcessor.h"

namespace xoceanus {

juce::AudioProcessorEditor* XOceanusProcessor::createEditor()
{
    return new XOceanusEditor(*this);
}

} // namespace xoceanus
