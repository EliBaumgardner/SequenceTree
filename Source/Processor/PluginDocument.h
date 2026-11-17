#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>
#include "Processor/Document/GraphState.h"
#include "Processor/Document/TraversalRuleState.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

class PluginDocument
{
public:

    PluginDocument();

    juce::ValueTree createState(const juce::ValueTree& parameters) const;

    void restore(const juce::ValueTree& restoredTree);

    static constexpr int schemaVersion = 1;

    juce::UndoManager undoManager;
    juce::UndoManager ornamentUndoManager;

    GraphState         graphState;
    TraversalRuleState traversalRuleState;

    OrnamentLibraries ornamentLibraries;

    juce::ValueTree colourPresets  { ArsNovaUI::InterfaceIdentifiers::ColourPresets };
    juce::ValueTree canvasSettings { ValueTreeIdentifiers::CanvasSettings };
    juce::ValueTree patch          { ValueTreeIdentifiers::Patch };

    juce::File userFolder;
    juce::File patchFolder;
};
