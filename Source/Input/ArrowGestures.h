#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/PopupWindow.h>
#include "../Util/ArrowInfo.h"

/*
    The arrow features of the canvas: drawing a new arrow, moving a dangling arrow's tip,
    removing an arrow, the right-click menu, and the hover highlight. Drawing and moving are one
    drag in three calls. beginMovingTip or beginTipFromNode starts it, dragTip follows the cursor
    and snaps the tip to a nearby node, and finishTip ends it by connecting to the snapped node,
    fixing a dangling arrow's tip, or keeping the new arrow dangling. Arrows are drawn from a
    node only in arrowMode, or from a traversal flag, whose arrow is kept only if it lands on a
    node.

    updateHover bolds the arrow under the cursor and fades in the arrows of nearby flags.
    removeArrow deletes a dangling arrow, a flag, or the connection between two nodes. The context
    menu opens the allowed-traversals window and toggles an arrow's traversal type and sync,
    through setArrowType and setArrowSync, after resolveOwnership finds which node the arrow
    belongs to.

    NodeController decides when a press belongs here and uses the grab radii for its hit tests;
    Titlebar sets arrowMode. Every edit goes through GraphState and NodeFactory in its own undo
    transaction.
*/

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

    void setArrowType(const Arrow* arrow, ArrowInfo::Type arrowType);
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

    NodeCanvas&                    nodeCanvas;
    GraphState&                    graphState;
    juce::UndoManager&             undoManager;
    ArsNovaUI::PopupWindowLauncher allowedTraversalsLauncher { "Allowed Traversals" };

    TipGesture tipGesture = TipGesture::None;

    juce::Component::SafePointer<Arrow> draggingDanglingArrow;
    juce::Component::SafePointer<Node>  tipSnapTarget;
    juce::Component::SafePointer<Node>  newArrowSourceNode;
};
