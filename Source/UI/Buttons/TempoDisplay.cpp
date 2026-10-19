#include "TempoDisplay.h"
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/RTData.h"

TempoDisplay::TempoDisplay(juce::UndoManager& undoManager)
    : editor(undoManager)
{
    auto tempoFormat = std::make_unique<ArsNovaUI::NumberFormat>(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier,
                                                                 ArsNovaUI::ValueFormat::editableDecimalPlaces);

    setTooltip("Tempo Multiplier");

    tempoFormat->suffix = "x";

    editor.setFormat(std::move(tempoFormat));
    editor.setJustification(juce::Justification::centredLeft);

    addAndMakeVisible(editor);
}

void TempoDisplay::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel   = ArsNovaUI::CustomLookAndFeel::get(*this);
    const auto                    contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio));

    lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());
    lookAndFeel.drawMetronomeIcon(graphics, contentBounds.withWidth(contentBounds.getHeight()).toFloat(), ArsNovaUI::ButtonState {});
}

void TempoDisplay::resized()
{
    auto contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio));

    contentBounds.removeFromLeft(contentBounds.getHeight());

    editor.setBounds(contentBounds);
}
