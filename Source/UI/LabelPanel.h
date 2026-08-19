//
// Created by Eli Baumgardner on 8/16/26.
//

#ifndef SEQUENCETREE_LABELPANEL_H
#define SEQUENCETREE_LABELPANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../util/ApplicationContext.h"
#include "../UI/Buttons/FileLabel.h"


class LabelPanel : public juce::Component {
public:

    LabelPanel(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void addFileLabel(juce::String fileName);
    void removeFileLabel(const FileLabel* label);

    std::vector<std::unique_ptr<FileLabel>> labels;

    static constexpr float labelAspectRatio = 0.3f;

private:

    int labelIndexAt(int y) const;

    int labelHeight  = 0;
    int draggedIndex = -1;

    ApplicationContext& context;
};


#endif //SEQUENCETREE_LABELPANEL_H