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
#include <unordered_map>

#include "../../Util/NodeInfo.h"
#include "../../Util/ApplicationContext.h"
#include "../Buttons/IconButton.h"
#include "../Editors/ValueEditor.h"


class Arrow;

class NodeCanvas;

struct NodeVisual {
    juce::Rectangle<float> bounds;
    juce::Colour colour;
    const std::map<int, juce::Colour>& highlights;
    bool isHovered  = false;
    bool isSelected = false;
    bool isOutlined = false;
    bool isEncapsulationRinged = false;
    bool hasInnerRim  = false;
    juce::Colour encapsulationRingColour;
};

class Node : public juce::Component {

public:

    explicit Node(const ApplicationContext& context);

    void paint  (juce::Graphics& g) override;
    void resized() override;

    void layoutInterior(juce::Rectangle<int> nodeSquare);

    NodeVisual getNodeVisual(juce::Rectangle<float> bounds) const {
        return { bounds, nodeColour, activeHighlights, isHovered, isSelected, isOutlined,
                 isEncapsulationRinged, hasInnerRim, encapsulationRingColour };
    }

    NodeVisual getNodeVisual() const { return getNodeVisual(getLocalBounds().toFloat()); }

    void setHoverVisual    (bool isHovered);
    void setSelectVisual   (bool isSelected);
    void setSelectVisual   ();
    void setHighlightVisual(int runId, bool isHighlighted, juce::Colour colour);
    void advancePulse(double frameSec);

    std::function<void(Node*, bool)> onSelected;

    virtual juce::Point<int> getNodeCentre() const { return getBounds().getCentre(); }

    virtual float getVisualRadius() const { return getHeight() * 0.5f; }

    virtual float getBodyExtent(juce::Point<float> approachDirection) const;

    virtual void bindToTree();
    virtual void bindValueEditorForMode();

    void setDisplayMode(NodeDisplayMode mode);
    void incrementNodeValue(int incrementValue);
    void refreshValueDisplay();

    std::unordered_multimap<int, Arrow*> nodeArrows;

    juce::ValueTree nodeValueTree;
    juce::ValueTree midiNoteData;

    NodeDisplayMode mode = NodeDisplayMode::Pitch;
    ValueEditor nodeValueEditor;

    std::unique_ptr<IconButton> upButton;
    std::unique_ptr<IconButton> downButton;

    ValueEditor countEditor;
    ValueEditor switchCountEditor;
    ValueEditor subLoopLimitEditor;

    juce::Colour nodeColour = juce::Colour::fromRGB(195,174,132).darker().darker().darker();
    juce::Colour encapsulationRingColour = juce::Colour::fromRGB(195,174,132).darker().darker().darker();

    int nodeId = -1;
    NodeType nodeType    = NodeType::Node;
    float incomingAngle  = 0.0f;

    bool isHovered           = false;
    bool isSelected          = false;
    bool isOutlined          = false;
    bool isEncapsulated      = false;
    bool isEncapsulationExit = false;
    bool isEncapsulationRinged = false;
    bool hasInnerRim  = false;
    bool isHighlighted       = false;
    std::map<int, juce::Colour> activeHighlights;
    std::set<int>               pendingHighlightOffIds;
    bool isAlternativeNode   = false;

    float pulsePhase         = 1.0f;
    double lastPulseFrameSec = 0.0;
    juce::VBlankAttachment pulseFrames;

    int displayCurrentCount = 0;
    int displayCountLimit   = 1;

    const float pulseRatePerSecond = 4.2f;

    const float nodeEditorWidthFactor = 0.45f;
    const float nodeEditorHeightFactor = 0.30f;

    const int   editorAreaBoundsReduction   = 3;
    const float incrementButtonHeightFactor = 0.25f;
    const float nodeValueTextInsetRatio     = 0.167f;

protected:
    const ApplicationContext& applicationContext;
};
