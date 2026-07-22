/*
  ==============================================================================

    Node.h
    Created: 6 May 2025 8:37:02pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <set>

#include "../../Util/NodeInfo.h"
#include "../../Util/ApplicationContext.h"
#include "../Buttons/IconButton.h"
#include "../Theme/NodeVisual.h"
#include "ValueEditor.h"


class Arrow;

class NodeCanvas;

class Node : public juce::Component, public juce::Timer {

public:

    Node(ApplicationContext& context);

    void paint  (juce::Graphics& g) override;
    void resized() override;

    NodeVisual getNodeVisual(juce::Rectangle<float> bounds) const {
        return { bounds, nodeColour, activeHighlights, isHovered, isSelected };
    }

    NodeVisual getNodeVisual() const { return getNodeVisual(getLocalBounds().toFloat()); }

    void setHoverVisual    (bool isHovered);
    void setSelectVisual   (bool isSelected);
    void setSelectVisual   ();
    void setHighlightVisual(int traversalId, bool isHighlighted, juce::Colour colour);

    std::function<void(Node*, bool)> onSelected;
    void timerCallback() override;

    virtual juce::Point<int> getNodeCentre() const { return getBounds().getCentre(); }

    virtual void setDisplayMode(NodeDisplayMode mode);
    void incrementNodeValue(int incrementValue);
    void refreshValueDisplay();

    Arrow* nodeArrow = nullptr;
    std::unordered_map<int, Arrow*> nodeArrows;

    juce::ValueTree nodeValueTree;
    juce::ValueTree midiNoteData;

    NodeDisplayMode mode;
    ValueEditor nodeValueEditor;

    std::unique_ptr<IconButton> upButton;
    std::unique_ptr<IconButton> downButton;

    ValueEditor countEditor;
    ValueEditor switchCountEditor;
    ValueEditor subLoopLimitEditor;

    juce::Colour nodeColour = juce::Colour::fromRGB(195,174,132).darker().darker().darker();

    int nodeId;
    NodeType nodeType    = NodeType::Node;
    float incomingAngle  = 0.0f;

    bool isHovered           = false;
    bool isSelected          = false;
    bool isHighlighted       = false;
    std::map<int, juce::Colour> activeHighlights;
    std::set<int>               pendingHighlightOffIds;
    bool isAlternativeNode   = false;

    float pulsePhase         = 1.0f;

    int displayCurrentCount = 0;
    int displayCountLimit   = 1;

    const float nodeEditorWidthFactor = 0.45f;
    const float nodeEditorHeightFactor = 0.30f;

    const int editorAreaBoundsReduction = 6;

protected:
    ApplicationContext& applicationContext;
};
