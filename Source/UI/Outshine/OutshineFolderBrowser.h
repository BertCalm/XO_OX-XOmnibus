#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include "../GalleryColors.h"

namespace xolokun {

struct FolderEntry
{
    juce::File   file;
    bool         isDirectory { false };
    juce::String durationStr;
    bool         selected { false };
};

class OutshineFolderBrowser : public juce::Component,
                              private juce::ListBoxModel
{
public:
    std::function<void(const juce::StringArray&)> onFilesConfirmed;

    OutshineFolderBrowser()
    {
        formatManager.registerBasicFormats();
        setWantsKeyboardFocus(true);
        A11y::setup(*this, "File Browser", "Navigate folders and select WAV files");

        pathLabel.setFont(GalleryFonts::value(11.0f));
        pathLabel.setColour(juce::Label::textColourId,
                            GalleryColors::get(GalleryColors::textDark()));
        addAndMakeVisible(pathLabel);

        backButton.setButtonText(juce::String(juce::CharPointer_UTF8("\xe2\x86\x90")));
        A11y::setup(backButton, "Back", "Navigate to parent directory");
        backButton.onClick = [this]() {
            if (currentPath.getParentDirectory() != currentPath)
                navigateTo(currentPath.getParentDirectory());
        };
        addAndMakeVisible(backButton);

        fileList.setModel(this);
        fileList.setRowHeight(kRowH);
        fileList.setMultipleSelectionEnabled(true);
        fileList.setColour(juce::ListBox::backgroundColourId,
                           GalleryColors::get(GalleryColors::slotBg()));
        A11y::setup(fileList, "Files", "List of files and folders");
        addAndMakeVisible(fileList);

        navigateTo(juce::File::getSpecialLocation(juce::File::userHomeDirectory));
    }

    ~OutshineFolderBrowser() override { fileList.setModel(nullptr); }

    void setRootPath(const juce::File& directory) { navigateTo(directory); }

    juce::StringArray getSelectedFilePaths() const
    {
        juce::StringArray result;
        auto selected = fileList.getSelectedRows();
        for (int i = 0; i < selected.size(); ++i)
        {
            int row = selected[i];
            if (row >= 0 && row < (int)entries.size() && !entries[(size_t)row].isDirectory)
                result.add(entries[(size_t)row].file.getFullPathName());
        }
        return result;
    }

    void selectAllWavFiles()
    {
        fileList.deselectAllRows();
        for (int i = 0; i < (int)entries.size(); ++i)
            if (!entries[(size_t)i].isDirectory)
                fileList.selectRow(i, true, false);
    }

    void clearSelection() { fileList.deselectAllRows(); }

    void paint(juce::Graphics& g) override
    {
        g.fillAll(GalleryColors::get(GalleryColors::shellWhite()));
    }

    void resized() override
    {
        auto area = getLocalBounds();
        auto pathBar = area.removeFromTop(kPathBarH);
        backButton.setBounds(pathBar.removeFromLeft(kPathBarH).reduced(4));
        pathLabel.setBounds(pathBar.reduced(4, 4));
        fileList.setBounds(area);
    }

private:
    int getNumRows() override { return (int)entries.size(); }

    void paintListBoxItem(int row, juce::Graphics& g, int w, int h, bool rowSelected) override
    {
        if (row < 0 || row >= (int)entries.size()) return;
        const auto& entry = entries[(size_t)row];

        if (rowSelected)
            g.fillAll(GalleryColors::get(GalleryColors::xoGold).withAlpha(0.18f));
        else
            g.fillAll(GalleryColors::get(GalleryColors::slotBg()));

        juce::String icon = entry.isDirectory
            ? juce::String(juce::CharPointer_UTF8("\xf0\x9f\x93\x81"))
            : juce::String(juce::CharPointer_UTF8("\xf0\x9f\x8e\xb5"));
        g.setFont(GalleryFonts::body(12.0f));
        g.setColour(GalleryColors::get(GalleryColors::textMid()));
        g.drawText(icon, 4, 0, 24, h, juce::Justification::centredLeft);

        g.setColour(GalleryColors::get(GalleryColors::textDark()));
        g.setFont(GalleryFonts::body(12.0f));
        g.drawText(entry.file.getFileName(), 32, 0, w - 80, h,
                   juce::Justification::centredLeft, true);

        if (!entry.isDirectory)
        {
            if (entries[(size_t)row].durationStr.isEmpty())
                entries[(size_t)row].durationStr = getFormattedDuration(entry.file);

            g.setColour(GalleryColors::get(GalleryColors::textMid()));
            g.setFont(GalleryFonts::value(10.0f));
            g.drawText(entries[(size_t)row].durationStr,
                       w - 72, 0, 68, h, juce::Justification::centredRight);
        }

        if (fileList.isRowSelected(row) && fileList.hasKeyboardFocus(true))
        {
            g.setColour(GalleryColors::get(GalleryColors::xoGold));
            g.drawRect(0, 0, w, h, 1);
        }
    }

    void listBoxItemClicked(int row, const juce::MouseEvent& e) override
    {
        if (row < 0 || row >= (int)entries.size()) return;
        const auto& entry = entries[(size_t)row];
        if (entry.isDirectory) return;

        if (e.mods.isShiftDown() && lastClickedRow >= 0)
        {
            int lo = juce::jmin(row, lastClickedRow);
            int hi = juce::jmax(row, lastClickedRow);
            fileList.deselectAllRows();
            for (int i = lo; i <= hi; ++i)
                if (!entries[(size_t)i].isDirectory)
                    fileList.selectRow(i, true, false);
        }
        else if (e.mods.isCommandDown())
        {
            if (fileList.isRowSelected(row))
                fileList.deselectRow(row);
            else
                fileList.selectRow(row, true, false);
        }
        else
        {
            fileList.selectRow(row, false, true);
        }
        lastClickedRow = row;
    }

    void listBoxItemDoubleClicked(int row, const juce::MouseEvent&) override
    {
        if (row < 0 || row >= (int)entries.size()) return;
        const auto& entry = entries[(size_t)row];

        if (entry.isDirectory)
            navigateTo(entry.file);
        else if (onFilesConfirmed)
            onFilesConfirmed({ entry.file.getFullPathName() });
    }

    void navigateTo(const juce::File& directory)
    {
        currentPath = directory;
        loadDirectory(directory);
    }

    void loadDirectory(const juce::File& directory)
    {
        entries.clear();

        if (directory.getParentDirectory() != directory)
            entries.push_back({ directory.getParentDirectory(), true, "..", false });

        juce::RangedDirectoryIterator dirIt(directory, false, "*", juce::File::findDirectories);
        for (auto& entry : dirIt)
            entries.push_back({ entry.getFile(), true, {}, false });

        juce::RangedDirectoryIterator wavIt(directory, false, "*.wav;*.WAV", juce::File::findFiles);
        for (auto& entry : wavIt)
            entries.push_back({ entry.getFile(), false, {}, false });

        pathLabel.setText(directory.getFullPathName(), juce::dontSendNotification);
        fileList.deselectAllRows();
        fileList.updateContent();
    }

    juce::String getFormattedDuration(const juce::File& wavFile) const
    {
        std::unique_ptr<juce::AudioFormatReader> reader(
            formatManager.createReaderFor(wavFile));
        if (!reader) return "--:--";

        double durationS = (double)reader->lengthInSamples / reader->sampleRate;
        int mins = (int)(durationS / 60.0);
        double secs = durationS - mins * 60.0;
        return juce::String(mins) + ":" + juce::String(secs, 1).paddedLeft('0', 4);
    }

    juce::File                       currentPath;
    mutable std::vector<FolderEntry> entries;
    juce::ListBox                    fileList;
    juce::Label                      pathLabel;
    juce::TextButton                 backButton { "<" };
    mutable juce::AudioFormatManager formatManager;
    int                              lastClickedRow { -1 };

    static constexpr int kPathBarH = 32;
    static constexpr int kRowH     = 28;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OutshineFolderBrowser)
};

} // namespace xolokun
