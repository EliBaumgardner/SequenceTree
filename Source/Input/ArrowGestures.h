#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/PopupWindow.h>
#include "../Util/ArrowInfo.h"

class Node;
class Arrow;
class NodeCanvas;
class GraphState;

class ArrowGestures
{
public:

    ArrowGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager);

    void beginMovingTip(Arrow& danglingArrow);
    void dragTip       (juce::Point<int> cursor);
    void finishTip     ();

    void updateHover         (juce::Point<float> cursor);
    void removeArrow         (Arrow& arrow);
    void showArrowContextMenu(Arrow* arrow);
    void setArrowMode        (bool enabled);

    bool beginTipFromNode(Node& node);

    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;
    static constexpr float arrowLabelGrabRadius    = 10.0f;

    bool arrowMode = false;

private:

    struct ArrowOwnership;

    Node* findTipSnapTarget(const Node& startNode, juce::Point<int> tip) const;

    void setArrowType(const Arrow* arrow, ArrowType arrowType);
    void setArrowSync(const Arrow* arrow, bool shouldSync);

    bool canBeTraversalArrow    (const Arrow* arrow) const;
    bool connectsToModulatorRoot(const Arrow* arrow) const;

    juce::ValueTree connectionTreeFor(const Arrow* arrow) const;

    ArrowOwnership resolveOwnership(const Arrow* arrow) const;

    enum class TipGesture { None, MovingDanglingTip, CreatingDanglingArrow, ConnectingFlag };

    struct ArrowOwnership
    {
        int ownerNodeId;
        int childNodeId;
    };

    static constexpr float flagProximityRadius = 28.0f;
    static constexpr float flagSnapThreshold   = 20.0f;

    NodeCanvas&         nodeCanvas;
    GraphState&         graphState;
    juce::UndoManager&  undoManager;
    PopupWindowLauncher allowedTraversalsLauncher { "Allowed Traversals" };

    TipGesture tipGesture = TipGesture::None;

    juce::Component::SafePointer<Arrow> draggingDanglingArrow;
    juce::Component::SafePointer<Node>  tipSnapTarget;
    juce::Component::SafePointer<Node>  newArrowSourceNode;
};
