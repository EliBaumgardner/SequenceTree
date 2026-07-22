//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_ICONBUTTON_H
#define SEQUENCETREE_ICONBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ButtonState.h"

class IconButton : public juce::Component, public juce::SettableTooltipClient {

public:

    using Painter = std::function<void(juce::Graphics&, juce::Rectangle<float>, const ButtonState&)>;

    std::function<void()> onClick;
    std::function<void()> onRightClick;

    explicit IconButton(Painter painter, juce::LookAndFeel* lookAndFeel = nullptr)
        : painter(std::move(painter))
    {
        jassert(this->painter != nullptr);

        if (lookAndFeel != nullptr) {
            setLookAndFeel(lookAndFeel);
        }
    }

    bool isSelected() const { return state.isSelected; }

    const juce::String& getText() const { return state.text; }

    void setText(juce::String newText)
    {
        if (state.text == newText) {
            return;
        }
        state.text = std::move(newText);
        repaint();
    }

    void setSelected(bool shouldBeSelected)
    {
        if (state.isSelected == shouldBeSelected) {
            return;
        }
        state.isSelected = shouldBeSelected;
        repaint();
    }

    void paint(juce::Graphics& g) override
    {
        if (painter) {
            painter(g, getLocalBounds().toFloat(), state);
        }
    }

    void mouseEnter(const juce::MouseEvent&) override { state.isHovered = true;  repaint(); }
    void mouseExit (const juce::MouseEvent&) override { state.isHovered = false; repaint(); }

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isRightButtonDown()) {
            if (onRightClick) {
                onRightClick();
            }
            return;
        }

        state.isDown = true;
        repaint();
    }

    void mouseUp(const juce::MouseEvent& e) override
    {
        const bool wasDown = state.isDown;
        state.isDown = false;
        repaint();

        if (wasDown && onClick && getLocalBounds().contains(e.getPosition())) {
            onClick();
        }
    }

    void toggleSelected() { setSelected(! state.isSelected); }

private:

    Painter painter;
    ButtonState state;
};

#endif //SEQUENCETREE_ICONBUTTON_H
