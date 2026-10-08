/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_data_structures/juce_data_structures.h>
#include "../Util/NodeInfo.h"
#include "../UI/PopupWindow.h"
#include "NodeCreationDispatcher.h"
#include "ConnectionOps.h"
#include "SelectionOps.h"


class NodeCanvas;

class Node;

class Modulator;

class NodeMenu;

class NodeData;

class NodeFactory;

class Arrow;

class GraphState;

class TraversalSession;

class RTGraphBuilder;



class NodeController : public juce::MouseListener {

public:

    using NodeControllerMode = NodeCreationMode;
    NodeControllerMode nodeControllerMode = NodeControllerMode::Node;
    SelectionOps       selectionOps;

    NodeController(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager, TraversalSession& traversalSession,
                   RTGraphBuilder& rtGraphBuilder);
    ~NodeController() override;

    void mouseDown           (const juce::MouseEvent& e) override;
    void mouseUp             (const juce::MouseEvent& e) override;
    void mouseEnter          (const juce::MouseEvent& e) override;
    void mouseExit           (const juce::MouseEvent& e) override;
    void mouseMove           (const juce::MouseEvent& e) override;
    void mouseDrag           (const juce::MouseEvent& e) override;

    void handleNodeDrag      (int nodeId, NodePosition newPosition);
    void handleNodeDragStart (Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods);

    void checkRootNodeSnap   (juce::Point<int> canvasPoint);
    void snapToGrid          (NodePosition &newPosition, juce::ValueTree draggedNodeTree);

    void showArrowContextMenu    (Arrow* arrow);
    void setArrowMode            (bool enabled);

private:

    enum class DragState {
        Idle,
        EditingValue,
        MovingArrowHead,
        MovingDanglingTip,
        ArrowSelected,
        CreatingDanglingArrow,
        ConnectingFlag,
        BoxSelecting
    };


    Node* findTipSnapTarget (const Node& startNode, juce::Point<int> tip) const;

    void updateArrowHover  (juce::Point<float> cursor);

    void dragValue             (const juce::MouseEvent& e);
    void dragArrowTip          (juce::Point<int> cursor);
    void endDrag ();

    void handleCanvasMouseDrag (const juce::MouseEvent& e);
    void handleNodeMouseDrag   (const juce::MouseEvent& e, Node& node);
    void handleNodeMouseDown   (const juce::MouseEvent& e, Node& node);
    void handleCanvasMouseDown (const juce::MouseEvent& e);

    void beginBoxSelection     (const juce::Point<int>& clickPoint);
    void updateBoxSelection    (const juce::MouseEvent& e);
    void setDraggedNodeVisible (bool shouldBeVisible);

    void connectWithSnapAnimation (int parentNodeId, int childNodeId, ArrowType rootConnectionType);
    void connectDraggedNodeToRoot ();

    void finishArrowTip              ();
    void finishArrowHeadDrag         ();
    void finishBoxSelection          ();

    void selectSpanNode   (Node& node);
    void showSelectionMenu(juce::Point<int> canvasPoint);

    bool isNodeCreationModeActive     () const;
    bool isArrowMode                  () const { return arrowMode; }
    bool toggleEncapsulationExpansion (Node& node);


    static constexpr float rootSnapThreshold       = 20.0f;
    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float flagProximityRadius     = 28.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;
    static constexpr float arrowLabelGrabRadius    = 10.0f;
    static constexpr int   dragThreshold           = 5;

    GraphState&         graphState;
    juce::UndoManager&  undoManager;
    TraversalSession&   traversalSession;
    RTGraphBuilder&     rtGraphBuilder;
    NodeCanvas&         nodeCanvas;
    PopupWindowLauncher allowedTraversalsLauncher { "Allowed Traversals" };

    ConnectionOps connectionOps;

    DragState dragState = DragState::Idle;

    juce::Component::SafePointer<Arrow> draggingDanglingArrow;
    juce::Component::SafePointer<Node>  tipSnapTarget;
    juce::Component::SafePointer<Node>  newArrowSourceNode;
    juce::Component::SafePointer<Node>  snapTargetRoot;
    juce::Component::SafePointer<Node>  draggingArrowHeadNode;
    juce::Component::SafePointer<Node>  draggingValueNode;

    double dragStartValue = 0.0;

    juce::Point<float> dragParentCenter;
    juce::Point<int>  selectionAnchor;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;

    bool isDragStart = true;
    bool arrowMode   = false;
};
