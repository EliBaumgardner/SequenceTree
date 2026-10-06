#include "ValueSlider.h"
#include "../Theme/CustomLookAndFeel.h"

ValueSlider::ValueSlider()
{
    setRepaintsOnMouseActivity(true);
}

void ValueSlider::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel::get(*this).drawValueSlider(graphics, *this);
}

void ValueSlider::mouseDown(const juce::MouseEvent& event)
{
    const auto  track      = getLocalBounds().toFloat().reduced(Theme::popupMenuItemInset, Theme::popupMenuItemGap);
    const float proportion = juce::jlimit(0.0f, 1.0f, (event.position.x - track.getX()) / juce::jmax(1.0f, track.getWidth()));

    boundValue.setValue(range.snapToLegalValue(range.convertFrom0to1(proportion)));

    repaint();

    if (onValueChange) {
        onValueChange();
    }
}

void ValueSlider::mouseDrag(const juce::MouseEvent& event)
{
    const auto  track      = getLocalBounds().toFloat().reduced(Theme::popupMenuItemInset, Theme::popupMenuItemGap);
    const float proportion = juce::jlimit(0.0f, 1.0f, (event.position.x - track.getX()) / juce::jmax(1.0f, track.getWidth()));
    const auto  dragged    = range.snapToLegalValue(range.convertFrom0to1(proportion));

    if (dragged == static_cast<double>(boundValue.getValue())) {
        return;
    }

    boundValue.setValue(dragged);

    repaint();

    if (onValueChange) {
        onValueChange();
    }
}
