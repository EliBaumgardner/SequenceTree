#pragma once

#include <ArsNovaUI/Theme/Visuals.h>

#include <juce_data_structures/juce_data_structures.h>
#include <juce_graphics/juce_graphics.h>

#include <vector>

class OrnamentSymbol
{
public:

    static void addStroke(juce::ValueTree ornament, const juce::Path& stroke, float width, juce::UndoManager* undoManager);
    static void erase    (juce::ValueTree ornament, juce::Point<float> centre, float radius, juce::UndoManager* undoManager);

    static std::vector<ArsNovaUI::SymbolStroke> read(const juce::String& symbol);

    static constexpr float defaultStrokeWidth = 0.06f;
};
