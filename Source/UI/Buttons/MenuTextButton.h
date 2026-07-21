//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_MENUTEXTBUTTON_H
#define SEQUENCETREE_MENUTEXTBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class MenuTextButton : public juce::Component {

public:

    MenuTextButton(ApplicationContext& context, juce::String buttonText)
        : text(std::move(buttonText))
    {
        setLookAndFeel(context.lookAndFeel);
    }

    ~MenuTextButton() override { setLookAndFeel(nullptr); }

    static constexpr int preferredHeight = 22;
    static constexpr int menuEdgeInset   = 6;

    const juce::String& getText() const { return text; }
    bool isHovered() const { return hovered; }
    bool isPressed() const { return pressed; }

    std::function<void()> onClick;

    void paint(juce::Graphics& g) override
    {
        const Theme& theme = CustomLookAndFeel::get(*this);
        auto bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);

        juce::Colour fill = theme.buttonColour;

        if (pressed) {
            fill = theme.buttonColour.darker();
        }
        else if (hovered) {
            fill = theme.buttonColour.brighter();
        }

        g.setColour(fill);
        g.fillRoundedRectangle(bounds, Theme::paneCornerRadius);

        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawRoundedRectangle(bounds, Theme::paneCornerRadius, 1.0f);

        g.setColour(juce::Colours::black.withAlpha(0.8f));
        g.setFont(juce::Font(juce::FontOptions(Theme::labelFontHeight)));
        g.drawText(text, getLocalBounds(), juce::Justification::centred);
    }

    void mouseEnter(const juce::MouseEvent& e) override
    {
        hovered = true;
        repaint();
    }

    void mouseExit(const juce::MouseEvent& e) override
    {
        hovered = false;
        repaint();
    }

    void mouseDown(const juce::MouseEvent& e) override
    {
        pressed = true;
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        pressed = false;
        repaint();

        if (onClick && getLocalBounds().contains(e.getPosition())) {
            onClick();
        }
    }

private:

    juce::String text;
    bool hovered = false;
    bool pressed = false;
};

#endif //SEQUENCETREE_MENUTEXTBUTTON_H
