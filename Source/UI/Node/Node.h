/*
  ==============================================================================

    Node.h
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Util/ValueTreeState.h"
#include "../Util/NodeInfo.h"
#include "Buttons/IncrementButton.h""


class NodeTextEditor;

class  NodeCanvas;

class Node : public juce::Component {
    
public:

    Node();

    void paint  (juce::Graphics& g) override;
    void resized() override;

    void setHoverVisual    (bool isHovered);
    void setSelectVisual   (bool isSelected);
    void setSelectVisual   ();
    void setHighlightVisual(bool isHighlighted);

    void setDisplayMode(NodeDisplayMode mode);

    juce::ValueTree nodeValueTree;
    juce::ValueTree midiNoteData;

    NodeDisplayMode mode;
    std::unique_ptr<NodeTextEditor> nodeTextEditor = nullptr;

    IncrementButton upButton { true };
    IncrementButton downButton { false };

    juce::Colour nodeColour = juce::Colour::fromRGB(91,86,76);

    bool isHovered     = false;
    bool isSelected    = false;
    bool isHighlighted = false;
};
