#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <optional>
#include "IconButton.h"

class ButtonPane : public juce::Component
{
public:

    struct Grid
    {
        int cellWidth;
        int cellHeight;
        int gap;
        int edgeInset;
    };

    enum class Selection { Momentary, Exclusive, ExclusiveOrNone };

    std::function<void(const IconButton*)> onSelectionChanged;

    Selection           selection = Selection::Momentary;
    std::optional<Grid> gridLayout;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    IconButton& addButton(IconButton::Icon icon, const juce::String& tooltip, std::function<void()> onClick = nullptr);
    void setSelectedButton(const IconButton* selected);
    int  idealWidth(int height) const;

private:

    juce::OwnedArray<IconButton> buttons;

    const IconButton* selectedButton = nullptr;
};
