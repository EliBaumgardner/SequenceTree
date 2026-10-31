#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <set>

#include "../../Util/NodeInfo.h"
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Canvas/CanvasNode.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include <ArsNovaUI/Theme/Visuals.h>

/*
    The component that shows one node of the graph on the canvas, drawn as a circle. It holds the
    node's tree (nodeValueTree) and the tree of its MIDI note (midiNoteData), which its editors
    are bound to: a main editor for the property the NodeInfo::DisplayMode picks (pitch, velocity, count
    limit, channel, repeat value or probability; bindValueEditorForMode binds it), and small
    badge editors for the count limit, the switch count limit and the sub-loop limit.

    It derives from ArsNovaUI::CanvasNode, which holds the node's id and tree, the arrows that
    start at it, its hover and selection, and where its body is. Here it also holds how the node
    looks right now: outlined, in an encapsulation group, and highlighted. activeHighlights maps
    the id of each traversal run that is on the node to its colour, and while a highlight begins,
    advancePulse animates a pulse once per display frame. setHoverVisual also fades in the arrows
    that leave a traversal flag. respondToClick starts editing the editor under a click.

    RootNode, Modulator, TraversalFlagNode and Encapsulator derive from it and change the shape
    or the editors. NodeManager creates every node and sets its id, trees and colour;
    NodeController and the gesture classes route the mouse to it.
*/

class NodeCanvas;

class Node : public ArsNovaUI::CanvasNode
{
public:

    static constexpr float pulseRatePerSecond          = 4.2f;
    static constexpr float nodeEditorWidthFactor       = 0.45f;
    static constexpr float nodeEditorHeightFactor      = 0.30f;
    static constexpr int   editorAreaBoundsReduction   = 3;
    static constexpr float incrementButtonHeightFactor = 0.25f;
    static constexpr float nodeValueTextInsetRatio     = 0.167f;

    explicit Node(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void incrementNodeValue(int incrementValue);
    void setDisplayMode    (NodeInfo::DisplayMode mode);

    void setHoverVisual    (bool isHovered) override;
    void setHighlightVisual(int runId, bool isHighlighted, juce::Colour colour);
    void advancePulse      (double frameSec);

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void lookAndFeelChanged() override;

    virtual void bindToTree ();
    virtual void bindValueEditorForMode ();
    void         respondToClick(juce::Point<int> localPoint) override;

    ArsNovaUI::NodeVisual getNodeVisual(juce::Rectangle<float> bounds) const;

    juce::ValueTree midiNoteData;

    NodeInfo::DisplayMode mode = NodeInfo::DisplayMode::Pitch;

    ArsNovaUI::IconButton  upButton;
    ArsNovaUI::IconButton  downButton;
    ArsNovaUI::ValueEditor countEditor;
    ArsNovaUI::ValueEditor switchCountEditor;
    ArsNovaUI::ValueEditor subLoopLimitEditor;

    juce::Colour nodeColour;
    juce::Colour encapsulationRingColour;

    NodeInfo::Type nodeType = NodeInfo::Type::Node;

    bool isHighlighted         = false;
    bool isOutlined            = false;
    bool hasInnerRim           = false;
    bool isEncapsulationRinged = false;

    std::map<int, juce::Colour> activeHighlights;
    std::set<int>               pendingHighlightOffIds;

    float                  pulsePhase        = 1.0f;
    double                 lastPulseFrameSec = 0.0;
    juce::VBlankAttachment pulseFrames;

    int displayCurrentCount = 0;
    int displayCountLimit   = 1;

    int interiorLeftInset = 0;
};
