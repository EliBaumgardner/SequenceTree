#include "CustomLookAndFeel.h"
#include "../../Graph/OrnamentLibraries.h"

void CustomLookAndFeel::drawOrnamentSymbol(juce::Graphics& graphics, juce::Rectangle<float> bounds, const juce::String& symbol)
{
    const float                  side   = juce::jmin(bounds.getWidth(), bounds.getHeight());
    const juce::Rectangle<float> square = bounds.withSizeKeepingCentre(side, side);
    const juce::AffineTransform  toArea = juce::AffineTransform::scale(side).translated(square.getX(), square.getY());

    for (const SymbolStroke& stroke : OrnamentLibraries::readSymbol(symbol)) {
        graphics.strokePath(stroke.path, juce::PathStrokeType(side * stroke.width, juce::PathStrokeType::curved, juce::PathStrokeType::butt), toArea);
    }
}

void CustomLookAndFeel::drawNodeOrnament(juce::Graphics& graphics, juce::Rectangle<float> bounds, OrnamentMark mark, const juce::String& symbol)
{
    if (mark == OrnamentMark::Symbol) {
        graphics.setColour(textColour);

        drawOrnamentSymbol(graphics, bounds, symbol);

        return;
    }

    if (mark == OrnamentMark::Missing) {
        graphics.setColour(scriptErrorColour);
        graphics.setFont(font(FontStyle::SemiBold, bounds.getHeight()));
        graphics.drawFittedText("?", bounds.toNearestInt(), juce::Justification::centred, 1);
    }
}
