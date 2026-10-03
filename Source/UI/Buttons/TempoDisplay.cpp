#include "TempoDisplay.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/RTData.h"

TempoDisplay::TempoDisplay(const ApplicationContext& context)
    : editor(context)
{
    auto tempoFormat = std::make_unique<NumberFormat>(RTtraversal::minimumTempoMultiplier, RTtraversal::maximumTempoMultiplier,
                                                      ValueFormat::editableDecimalPlaces);

    setLookAndFeel(context.lookAndFeel);
    setTooltip("Tempo Multiplier");

    tempoFormat->suffix = "x";

    editor.setFormat(std::move(tempoFormat));

    addAndMakeVisible(editor);
}

void TempoDisplay::paint(juce::Graphics& graphics)
{
    const Theme& theme = CustomLookAndFeel::get(*this);

    graphics.setColour(theme.buttonBarColour);
    graphics.fillRoundedRectangle(editor.getBounds().toFloat(), Theme::paneCornerRadius);
}

void TempoDisplay::resized()
{
    editor.setBounds(getLocalBounds().reduced(juce::roundToInt(getHeight() * Theme::contentInsetRatio)));
}
