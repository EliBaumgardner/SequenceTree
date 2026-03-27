/*
  ==============================================================================

    ComponentContext.h
    Created: 2 Aug 2025 11:28:10am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include "../Util/ValueTreeState.h"

class SequenceTreeAudioProcessor;
class NodeCanvas;
class CustomLookAndFeel;
class NodeController;
class NodeFactory;
class ValueTreeState;



namespace ComponentContext
{
    inline SequenceTreeAudioProcessor* processor = nullptr;
    inline NodeCanvas* canvas                    = nullptr;
    inline CustomLookAndFeel* lookAndFeel        = nullptr;
    inline juce::UndoManager* undoManager        = nullptr;
    inline ValueTreeState* valueTreeState        = nullptr;
    inline NodeController* nodeController      = nullptr;
}
