/*
  ==============================================================================

    ComponentContext.h
    Created: 2 Aug 2025 11:28:10am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

class SequenceTreeAudioProcessor;
class NodeCanvas;
class CustomLookAndFeel;

namespace ComponentContext
{
    inline SequenceTreeAudioProcessor* processor = nullptr;
    inline NodeCanvas* canvas                    = nullptr;
    inline CustomLookAndFeel* lookAndFeel        = nullptr;
}
