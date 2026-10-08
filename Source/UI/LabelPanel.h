#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Editors/FileLabel.h"

#include <span>

class LabelPanel : public juce::Component
{
public:

    static constexpr float labelAspectRatio = 0.3f;
    static constexpr float labelGapRatio    = 0.12f;

    std::vector<std::unique_ptr<FileLabel>> labels;

    std::function<void(FileLabel*)>       onLabelClicked;
    std::function<void(int)>              onLabelRemoved;
    std::function<void(std::vector<int>)> onLabelsReordered;

    explicit LabelPanel(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void addFileLabel(juce::String fileName);
    void setSelectedLabel(const FileLabel* label);
    void removeFileLabel(const FileLabel* label);
    void applyOrder(std::span<const int> fileIds);

private:

    int labelIndexAt(int positionY) const;

    int  labelHeight  = 0;
    int  labelGap     = 0;
    int  draggedIndex = -1;
    bool orderChanged = false;

    juce::UndoManager& undoManager;
};
