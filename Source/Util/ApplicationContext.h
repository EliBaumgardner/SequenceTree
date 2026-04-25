/*
  ==============================================================================

    ApplicationContext.h
    Created: 24 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class SequenceTreeAudioProcessor;
class NodeCanvas;
class CustomLookAndFeel;
class NodeController;
class NodeFactory;
class ValueTreeState;

struct ApplicationContext
{
    SequenceTreeAudioProcessor* processor      = nullptr;
    NodeCanvas*                 canvas         = nullptr;
    CustomLookAndFeel*          lookAndFeel    = nullptr;
    juce::UndoManager*          undoManager    = nullptr;
    ValueTreeState*             valueTreeState = nullptr;
    NodeController*             nodeController = nullptr;
};
