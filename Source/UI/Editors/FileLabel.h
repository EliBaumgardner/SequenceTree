#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ValueEditor.h"
#include "../../Util/ApplicationContext.h"
#include "../Buttons/IconButton.h"

class FileLabel : public juce::Component
{
public:

    static constexpr float fileLabelInsetRatio = 0.06f;
    static constexpr float removeButtonRatio   = 0.7f;
    static constexpr float fileTextWidthRatio  = 0.5f;
    static constexpr float fileTextHeightRatio = 0.6f;

    std::function<void()> onRemove;
    std::function<void()> onMouseClicked;

    int  fileId   = -1;
    bool grabbed  = false;
    bool selected = false;

    ValueEditor fileText;
    IconButton  removeButton;

    explicit FileLabel(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;

    void setGrabbed(bool shouldBeGrabbed);
    void setSelected(bool shouldBeSelected);
};
