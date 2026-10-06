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

struct NodeVisual
{
    juce::Rectangle<float>             bounds;
    juce::Colour                       colour;
    const std::map<int, juce::Colour>& highlights;
    bool                               isHovered             = false;
    bool                               isSelected            = false;
    bool                               isOutlined            = false;
    bool                               isEncapsulationRinged = false;
    bool                               hasInnerRim           = false;
    juce::Colour                       encapsulationRingColour;
};

class Node : public juce::Component
{
public:

    static constexpr float pulseRatePerSecond          = 4.2f;
    static constexpr float nodeEditorWidthFactor       = 0.45f;
    static constexpr float nodeEditorHeightFactor      = 0.30f;
    static constexpr int   editorAreaBoundsReduction   = 3;
    static constexpr float incrementButtonHeightFactor = 0.25f;
    static constexpr float nodeValueTextInsetRatio     = 0.167f;

    static const juce::Colour defaultNodeColour;

    explicit Node(const ApplicationContext& context);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    NodeVisual getNodeVisual(juce::Rectangle<float> bounds) const;
    void incrementNodeValue(int incrementValue);
    void refreshValueDisplay();
    void setHoverVisual(bool isHovered);
    void setSelectVisual(bool isSelected);
    void setSelectVisual();
    void setHighlightVisual(int runId, bool isHighlighted, juce::Colour colour);
    void advancePulse(double frameSec);
    virtual juce::Point<int> getNodeCentre() const;
    virtual float            getVisualRadius() const;
    virtual float            getBodyExtent(juce::Point<float> approachDirection) const;
    virtual void bindToTree();
    virtual void bindValueEditorForMode();
    void setDisplayMode(NodeDisplayMode mode);
    void mouseMove(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;
    virtual void respondToClick(juce::Point<int> localPoint);

    std::function<void(Node*, bool)> onSelected;

    std::unordered_multimap<int, Arrow*> nodeArrows;

    juce::ValueTree nodeValueTree;
    juce::ValueTree midiNoteData;

    NodeDisplayMode mode = NodeDisplayMode::Pitch;

    ValueEditor nodeValueEditor;
    IconButton  upButton;
    IconButton  downButton;
    ValueEditor countEditor;
    ValueEditor switchCountEditor;
    ValueEditor subLoopLimitEditor;

    juce::Colour nodeColour              = defaultNodeColour;
    juce::Colour encapsulationRingColour = defaultNodeColour;

    int      nodeId        = -1;
    NodeType nodeType      = NodeType::Node;
    float    incomingAngle = 0.0f;

    bool isHovered             = false;
    bool isSelected            = false;
    bool isOutlined            = false;
    bool isEncapsulated        = false;
    bool isEncapsulationExit   = false;
    bool isEncapsulationRinged = false;
    bool hasInnerRim           = false;
    bool isHighlighted         = false;
    bool isAlternativeNode     = false;

    std::map<int, juce::Colour> activeHighlights;
    std::set<int>               pendingHighlightOffIds;

    float                  pulsePhase        = 1.0f;
    double                 lastPulseFrameSec = 0.0;
    juce::VBlankAttachment pulseFrames;

    int displayCurrentCount = 0;
    int displayCountLimit   = 1;
    int interiorLeftInset   = 0;

protected:

    const ApplicationContext& applicationContext;
};
