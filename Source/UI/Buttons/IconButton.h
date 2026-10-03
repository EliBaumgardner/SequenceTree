#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/Theme.h"

class CustomLookAndFeel;

struct ButtonState
{
    bool isHovered  = false;
    bool isDown     = false;
    bool isSelected = false;

    juce::String text;
};

class IconButton : public juce::Component, public juce::SettableTooltipClient
{
public:

    using Painter = std::function<void(juce::Graphics&, juce::Rectangle<float>, const ButtonState&)>;
    using Icon    = void (CustomLookAndFeel::*)(juce::Graphics&, juce::Rectangle<float>, const ButtonState&);

    static constexpr int captionHeight = 12;
    static constexpr int captionGap    = 2;

    std::function<void()> onClick;
    std::function<void()> onRightClick;

    Painter painter;
    Icon    icon = nullptr;

    ButtonState state;

    IconButton();

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void lookAndFeelChanged() override;
    void setText(juce::String newText);
    void setCaption(const juce::String& newCaption);
    void setSelected(bool shouldBeSelected);
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:

    juce::Label caption;
};
