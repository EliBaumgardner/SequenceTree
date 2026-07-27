//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_ICONBUTTON_H
#define SEQUENCETREE_ICONBUTTON_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/ButtonState.h"
#include "../Theme/Theme.h"

class IconButton : public juce::Component, public juce::SettableTooltipClient {

public:

    using Painter = std::function<void(juce::Graphics&, juce::Rectangle<float>, const ButtonState&)>;

    static constexpr int captionHeight = 12;
    static constexpr int captionGap    = 2;

    std::function<void()> onClick;
    std::function<void()> onRightClick;

    explicit IconButton(Painter painter, juce::LookAndFeel* lookAndFeel = nullptr)
        : painter(std::move(painter))
    {
        jassert(this->painter != nullptr);

        caption.setJustificationType(juce::Justification::centredTop);
        caption.setInterceptsMouseClicks(false, false);
        addChildComponent(caption);

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

    void setCaption(const juce::String& newCaption)
    {
        caption.setText(newCaption, juce::dontSendNotification);
        caption.setVisible(newCaption.isNotEmpty());
        resized();
    }

    bool hasCaption() const { return caption.isVisible(); }

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
            painter(g, getIconBounds().toFloat(), state);
        }
    }

    void resized() override
    {
        if (hasCaption()) {
            caption.setBounds(getLocalBounds().removeFromBottom(captionHeight));
        }
    }

    void lookAndFeelChanged() override
    {
        if (const auto* theme = dynamic_cast<const Theme*>(&getLookAndFeel())) {
            caption.setColour(juce::Label::textColourId, theme->getTextColour());
            caption.setFont(juce::Font(juce::FontOptions(Theme::labelFontHeight)));
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

    juce::Rectangle<int> getIconBounds() const
    {
        auto bounds = getLocalBounds();

        if (hasCaption()) {
            bounds.removeFromBottom(captionHeight + captionGap);
            bounds = bounds.withSizeKeepingCentre(juce::jmin(bounds.getWidth(), bounds.getHeight()),
                                                  bounds.getHeight());
        }

        return bounds;
    }

    const Painter painter;
    ButtonState   state;
    juce::Label   caption;
};

#endif //SEQUENCETREE_ICONBUTTON_H
