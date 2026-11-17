#include "OrnamentSymbol.h"
#include "TypedProperties.h"

#include <cmath>

void OrnamentSymbol::addStroke(juce::ValueTree ornament, const juce::Path& stroke, float width, juce::UndoManager* undoManager)
{
    OrnamentProperties properties(ornament);

    juce::String symbol = properties.symbol;

    if (symbol.isNotEmpty()) {
        symbol << "|";
    }

    symbol << juce::String(width) << " " << stroke.toString();

    properties.symbol.setValue(symbol, undoManager);
}

void OrnamentSymbol::erase(juce::ValueTree ornament, juce::Point<float> centre, float radius, juce::UndoManager* undoManager)
{
    juce::StringArray erasedStrokes;

    OrnamentProperties properties(ornament);

    const juce::String symbol = properties.symbol;

    for (const ArsNovaUI::SymbolStroke& stroke : read(symbol)) {
        juce::Path erased;

        juce::Point<float> current;

        bool penDown = false;

        juce::Path::Iterator element(stroke.path);

        while (element.next()) {
            const juce::Point<float> end(element.x1, element.y1);

            if (element.elementType != juce::Path::Iterator::lineTo) {
                current = end;
                penDown = false;

                continue;
            }

            const juce::Point<float> along    = end - current;
            const juce::Point<float> toCentre = centre - current;

            const float length = along.getDistanceFromOrigin();

            const juce::Point<float> direction = along / length;

            const float alongCentre  = toCentre.getDotProduct(direction);
            const float acrossCentre = std::abs(direction.x * toCentre.y - direction.y * toCentre.x);
            const float gap          = juce::jmax(0.0f, acrossCentre - stroke.width * 0.5f);

            float enter = 1.0f;
            float leave = 1.0f;

            if (length > 0.0f && gap < radius) {
                const float halfChord = std::sqrt(radius * radius - gap * gap);

                enter = (alongCentre - halfChord) / length;
                leave = (alongCentre + halfChord) / length;
            }

            if (leave <= 0.0f || enter >= 1.0f) {
                enter = 1.0f;
                leave = 1.0f;
            }

            if (enter > 0.0f && ! penDown) {
                erased.startNewSubPath(current);
            }

            if (enter > 0.0f) {
                const juce::Point<float> entryPoint = current + along * enter;

                erased.lineTo(entryPoint);
            }

            if (leave < 1.0f) {
                const juce::Point<float> exitPoint = current + along * juce::jmax(leave, 0.0f);

                erased.startNewSubPath(exitPoint);
                erased.lineTo(end);
            }

            penDown = enter >= 1.0f || leave < 1.0f;
            current = end;
        }

        if (! erased.isEmpty()) {
            erasedStrokes.add(juce::String(stroke.width) + " " + erased.toString());
        }
    }

    const juce::String erasedSymbol = erasedStrokes.joinIntoString("|");

    properties.symbol.setValue(erasedSymbol, undoManager);
}

std::vector<ArsNovaUI::SymbolStroke> OrnamentSymbol::read(const juce::String& symbol)
{
    std::vector<ArsNovaUI::SymbolStroke> strokes;

    for (const juce::String& entry : juce::StringArray::fromTokens(symbol, "|", "")) {
        const juce::String widthText = entry.upToFirstOccurrenceOf(" ", false, false);

        ArsNovaUI::SymbolStroke stroke { juce::Path(), defaultStrokeWidth };

        if (widthText.containsOnly("0123456789.")) {
            stroke.width = widthText.getFloatValue();

            stroke.path.restoreFromString(entry.fromFirstOccurrenceOf(" ", false, false));
        }
        else {
            stroke.path.restoreFromString(entry);
        }

        strokes.push_back(stroke);
    }

    return strokes;
}
