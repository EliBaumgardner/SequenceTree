/*
  ==============================================================================

    Node.h
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Util/NodeInfo.h"
#include "../Util/ApplicationContext.h"
#include "Buttons/IncrementButton.h"
#include "ValueEditor.h"


class NodeArrow;

class NodeTextEditor;

class NodeCanvas;

class Node : public juce::Component, public juce::Timer {

public:

    Node(ApplicationContext& context);

    void paint  (juce::Graphics& g) override;
    void resized() override;

    void setHoverVisual    (bool isHovered);
    void setSelectVisual   (bool isSelected);
    void setSelectVisual   ();
    void setHighlightVisual(bool isHighlighted);
    void timerCallback() override;

    // Returns the centre of the visual circle in canvas coordinates.
    // Overridden by RootNode, whose component extends left for the loop limit rectangle.
    virtual juce::Point<int> getNodeCentre() const { return getBounds().getCentre(); }

    virtual void setDisplayMode(NodeDisplayMode mode);
    void incrementNodeTextEditorValue(int incrementValue);

    NodeArrow* nodeArrow = nullptr;
    std::unordered_map<int, NodeArrow*> nodeArrows;

    juce::ValueTree nodeValueTree;
    juce::ValueTree midiNoteData;

    NodeDisplayMode mode;
    std::unique_ptr<NodeTextEditor> nodeTextEditor = nullptr;

    IncrementButton upButton   { true };
    IncrementButton downButton { false };
    ValueEditor countEditor;

    juce::Colour nodeColour = juce::Colour::fromRGB(91,86,76);

    int nodeId;
    NodeType nodeType    = NodeType::Node;
    float incomingAngle  = 0.0f;
    bool isHovered           = false;
    bool isSelected          = false;
    bool isHighlighted       = false;
    bool pendingHighlightOff = false;
    float pulsePhase         = 1.0f;

    int displayCurrentCount = 0;
    int displayCountLimit   = 1;

protected:
    ApplicationContext& applicationContext;
};
