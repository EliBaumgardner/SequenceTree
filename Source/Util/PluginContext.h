/*
  ==============================================================================

    ComponentContext.h
    Created: 2 Aug 2025 11:28:10am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>


class SequenceTreeAudioProcessor;
class NodeCanvas;
class CustomLookAndFeel;
class ValueTreeState;
class NodeController;


namespace ComponentContext
{
    inline SequenceTreeAudioProcessor* processor = nullptr;
    inline NodeCanvas* canvas                    = nullptr;
    inline CustomLookAndFeel* lookAndFeel        = nullptr;
    inline juce::UndoManager* undoManager        = nullptr;
    inline ValueTreeState* valueTreeState        = nullptr;
    inline NodeController* nodeController      = nullptr;
}
