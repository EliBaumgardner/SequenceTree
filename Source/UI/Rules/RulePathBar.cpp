#include "RulePathBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

void RulePathBar::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Font font = theme.font(ArsNovaUI::Theme::FontStyle::Regular, pathFontHeight);

    auto bounds = getLocalBounds().reduced(pathInset, 0);

    graphics.setColour(theme.surfaceColour);
    graphics.fillRect(getLocalBounds());

    graphics.setFont(font);

    for (int index = 0; index < segments.size(); ++index) {
        const int width = juce::GlyphArrangement::getStringWidthInt(font, segments[index]);

        if (index > 0) {
            graphics.setColour(theme.dimTextColour);
            graphics.drawText(juce::String::fromUTF8("\xe2\x80\xba"), bounds.removeFromLeft(separatorWidth), juce::Justification::centred, false);
        }

        if (index == segments.size() - 1) {
            graphics.setColour(theme.textColour);
        }
        else {
            graphics.setColour(theme.mutedTextColour);
        }

        graphics.drawText(segments[index], bounds.removeFromLeft(width), juce::Justification::centredLeft, true);
    }
}
