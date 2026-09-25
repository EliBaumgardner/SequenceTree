//
// Created by Eli Baumgardner on 8/16/26.
//

#ifndef SEQUENCETREE_LABELPANEL_H
#define SEQUENCETREE_LABELPANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Util/ApplicationContext.h"
#include "Editors/FileLabel.h"

#include <span>


class LabelPanel : public juce::Component {
public:

    LabelPanel(const ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void addFileLabel(juce::String fileName);
    void removeFileLabel(const FileLabel* label);
    void setSelectedLabel(const FileLabel* label);
    void applyOrder(std::span<const int> fileIds);

    std::vector<std::unique_ptr<FileLabel>> labels;

    std::function<void(FileLabel*)>        onLabelClicked;
    std::function<void(int)>               onLabelRemoved;
    std::function<void(std::vector<int>)>  onLabelsReordered;
    static constexpr float labelAspectRatio = 0.3f;
    static constexpr float labelGapRatio    = 0.12f;

private:

    int labelIndexAt(int y) const;

    int  labelHeight  = 0;
    int  labelGap     = 0;
    int  draggedIndex = -1;
    bool orderChanged = false;

    const ApplicationContext& context;
};


#endif //SEQUENCETREE_LABELPANEL_H