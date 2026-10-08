#include "TempoDisplay.h"
#include "IconButton.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/RTData.h"

TempoDisplay::TempoDisplay(juce::UndoManager& undoManager)
    : editor(undoManager)
{
    auto tempoFormat = std::make_unique<NumberFormat>(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier,
                                                      ValueFormat::editableDecimalPlaces);

    setTooltip("Tempo Multiplier");

    tempoFormat->suffix = "x";

    editor.setFormat(std::move(tempoFormat));
    editor.setJustification(juce::Justification::centredLeft);

    addAndMakeVisible(editor);
}

void TempoDisplay::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel   = CustomLookAndFeel::get(*this);
    const auto         contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * Theme::contentInsetRatio));

    lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());
    lookAndFeel.drawTempoIcon(graphics, contentBounds.withWidth(contentBounds.getHeight()).toFloat(), ButtonState {});
}

void TempoDisplay::resized()
{
    auto contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * Theme::contentInsetRatio));

    contentBounds.removeFromLeft(contentBounds.getHeight());

    editor.setBounds(contentBounds);
}
