#include "CustomLookAndFeel.h"

void CustomLookAndFeel::drawOrnamentSymbol(juce::Graphics& graphics, juce::Rectangle<float> bounds, const std::vector<SymbolStroke>& strokes)
{
    const float                  side   = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const juce::Rectangle<float> square = bounds.withSizeKeepingCentre(side, side);
    const juce::AffineTransform  toArea = juce::AffineTransform::scale(side).translated(square.getX(), square.getY());

    for (const SymbolStroke& stroke : strokes) {
        graphics.strokePath(stroke.path, juce::PathStrokeType(side * stroke.width, juce::PathStrokeType::curved, juce::PathStrokeType::butt), toArea);
    }
}

void CustomLookAndFeel::drawNodeOrnament(juce::Graphics& graphics, juce::Rectangle<float> bounds, OrnamentMark mark, const std::vector<SymbolStroke>& strokes)
{
    if (mark == OrnamentMark::Symbol) {
        graphics.setColour(textColour);

        drawOrnamentSymbol(graphics, bounds, strokes);

        return;
    }

    if (mark == OrnamentMark::Missing) {
        graphics.setColour(scriptErrorColour);
        graphics.setFont(font(FontStyle::SemiBold, bounds.getHeight()));
        graphics.drawFittedText("?", bounds.toNearestInt(), juce::Justification::centred, 1);
    }
}
