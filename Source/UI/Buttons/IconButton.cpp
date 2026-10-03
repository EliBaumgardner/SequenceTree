#include "IconButton.h"
#include "../Theme/CustomLookAndFeel.h"

IconButton::IconButton()
{
    caption.setJustificationType(juce::Justification::centredTop);
    caption.setInterceptsMouseClicks(false, false);

    addChildComponent(caption);
}

void IconButton::paint(juce::Graphics& graphics)
{
    auto iconBounds = getLocalBounds();

    if (caption.isVisible()) {
        iconBounds.removeFromBottom(captionHeight + captionGap);

        iconBounds = iconBounds.withSizeKeepingCentre(juce::jmin(iconBounds.getWidth(), iconBounds.getHeight()), iconBounds.getHeight());
    }

    if (icon != nullptr) {
        (CustomLookAndFeel::get(*this).*icon)(graphics, iconBounds.toFloat(), state);
    }

    if (painter) {
        painter(graphics, iconBounds.toFloat(), state);
    }
}

void IconButton::resized()
{
    if (caption.isVisible()) {
        caption.setBounds(getLocalBounds().removeFromBottom(captionHeight));
    }
}

void IconButton::lookAndFeelChanged()
{
    if (const auto* theme = dynamic_cast<const Theme*>(&getLookAndFeel())) {
        caption.setColour(juce::Label::textColourId, theme->textColour);
        caption.setFont(juce::Font(juce::FontOptions(Theme::labelFontHeight)));
    }
}

void IconButton::setText(juce::String newText)
{
    if (state.text == newText) {
        return;
    }

    state.text = std::move(newText);

    repaint();
}

void IconButton::setCaption(const juce::String& newCaption)
{
    caption.setText(newCaption, juce::dontSendNotification);
    caption.setVisible(newCaption.isNotEmpty());

    resized();
}

void IconButton::setSelected(bool shouldBeSelected)
{
    if (state.isSelected == shouldBeSelected) {
        return;
    }

    state.isSelected = shouldBeSelected;

    repaint();
}

void IconButton::mouseEnter(const juce::MouseEvent&)
{
    state.isHovered = true;

    repaint();
}

void IconButton::mouseExit(const juce::MouseEvent&)
{
    state.isHovered = false;

    repaint();
}

void IconButton::mouseDown(const juce::MouseEvent& event)
{
    if (event.mods.isRightButtonDown()) {
        if (onRightClick) {
            onRightClick();
        }

        return;
    }

    state.isDown = true;

    repaint();
}

void IconButton::mouseUp(const juce::MouseEvent& event)
{
    const bool wasDown = state.isDown;

    state.isDown = false;

    repaint();

    if (wasDown && onClick && getLocalBounds().contains(event.getPosition())) {
        onClick();
    }
}
