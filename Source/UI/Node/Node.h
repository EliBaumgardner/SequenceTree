#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <map>
#include <set>
#include <unordered_map>

#include "../../Util/NodeInfo.h"
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include <ArsNovaUI/Theme/Visuals.h>

class Arrow;
class NodeCanvas;

class Node : public juce::Component
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

    void incrementNodeValue (int incrementValue);
    void refreshValueDisplay();
    void setDisplayMode     (NodeDisplayMode mode);

    void setHoverVisual    (bool isHovered);
    void setSelectVisual   (bool isSelected);
    void setSelectVisual   ();
    void setHighlightVisual(int runId, bool isHighlighted, juce::Colour colour);
    void advancePulse      (double frameSec);

    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    void lookAndFeelChanged() override;

    virtual void bindToTree            ();
    virtual void bindValueEditorForMode();
    virtual void respondToClick        (juce::Point<int> localPoint);

    virtual juce::Point<int> getNodeCentre  () const;
    virtual float            getVisualRadius() const;
    virtual float            getBodyExtent  (juce::Point<float> approachDirection) const;

    ArsNovaUI::NodeVisual getNodeVisual(juce::Rectangle<float> bounds) const;

    std::function<void(Node*, bool)> onSelected;

    std::unordered_multimap<int, Arrow*> nodeArrows;

    juce::ValueTree nodeValueTree;
    juce::ValueTree midiNoteData;

    NodeDisplayMode mode = NodeDisplayMode::Pitch;

    ArsNovaUI::ValueEditor nodeValueEditor;
    ArsNovaUI::IconButton  upButton;
    ArsNovaUI::IconButton  downButton;
    ArsNovaUI::ValueEditor countEditor;
    ArsNovaUI::ValueEditor switchCountEditor;
    ArsNovaUI::ValueEditor subLoopLimitEditor;

    juce::Colour nodeColour;
    juce::Colour encapsulationRingColour;

    int      nodeId        = -1;
    NodeType nodeType      = NodeType::Node;
    float    incomingAngle = 0.0f;

    bool isHovered     = false;
    bool isSelected    = false;
    bool isHighlighted = false;

    bool isOutlined  = false;
    bool hasInnerRim = false;

    bool isEncapsulated        = false;
    bool isEncapsulationExit   = false;
    bool isEncapsulationRinged = false;

    bool isAlternativeNode = false;

    std::map<int, juce::Colour> activeHighlights;
    std::set<int>               pendingHighlightOffIds;

    float                  pulsePhase        = 1.0f;
    double                 lastPulseFrameSec = 0.0;
    juce::VBlankAttachment pulseFrames;

    int displayCurrentCount = 0;
    int displayCountLimit   = 1;

    int interiorLeftInset = 0;

protected:

    juce::UndoManager& undoManager;
};
