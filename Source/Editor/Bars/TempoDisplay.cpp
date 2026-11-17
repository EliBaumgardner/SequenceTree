#include "TempoDisplay.h"
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "Core/Model/RTData.h"

TempoDisplay::TempoDisplay(juce::UndoManager& undoManager)
    : editor(undoManager)
{
    auto tempoFormat = std::make_unique<ArsNovaUI::NumberFormat>(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier,
                                                                 ArsNovaUI::ValueFormat::editableDecimalPlaces);

    setTooltip("Tempo Multiplier");

    tempoFormat->suffix = "x";

    editor.setFormat(std::move(tempoFormat));
    editor.setJustification(juce::Justification::centredLeft);
    editor.setTooltip("Tempo Multiplier");

    addAndMakeVisible(editor);
}

void TempoDisplay::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel   = ArsNovaUI::CustomLookAndFeel::get(*this);
    const int                     contentInset  = juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio);
    const auto                    contentBounds = getLocalBounds().reduced(contentInset);

    lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());
    lookAndFeel.drawIcon(graphics, contentBounds.withWidth(contentBounds.getHeight()).toFloat(), ArsNovaUI::IconLibrary::get().metronome, ArsNovaUI::ButtonState {});
}

void TempoDisplay::resized()
{
    const int contentInset = juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio);

    auto contentBounds = getLocalBounds().reduced(contentInset);

    contentBounds.removeFromLeft(contentBounds.getHeight());

    editor.setBounds(contentBounds);
}
