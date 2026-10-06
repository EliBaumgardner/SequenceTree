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
#include "../Util/ApplicationContext.h"
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



class NodeController : public juce::MouseListener {

public:

    using NodeControllerMode = NodeCreationMode;
    NodeControllerMode nodeControllerMode = NodeControllerMode::Node;
    SelectionOps       selectionOps;

    NodeController (const ApplicationContext& context, NodeCanvas& canvas);
    ~NodeController() override;

    void mouseDown           (const juce::MouseEvent& e) override;
    void mouseUp             (const juce::MouseEvent& e) override;
    void mouseEnter          (const juce::MouseEvent& e) override;
    void mouseExit           (const juce::MouseEvent& e) override;
    void mouseMove           (const juce::MouseEvent& e) override;
    void mouseDrag           (const juce::MouseEvent& e) override;

    void handleNodeDrag      (juce::UndoManager *undoManager, int nodeId, NodePosition newPosition);
    void handleNodeDragStart (juce::UndoManager *undoManager, Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods);

    void checkRootNodeSnap   (juce::Point<int> canvasPoint);
    void snapToGrid          (juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree);

    void updateConnectionPreview (Node *node, const NodePosition& newPosition, bool dashed);
    void commitFlagConnection    (int sourceNodeId, Node* target);

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


    juce::Point<int> danglingTipFor (const Node* startNode, juce::Point<int> cursor);
    Node* findDanglingSnapTarget    (const Node* startNode, juce::Point<int> tip) const;

    void updateArrowHover  (juce::Point<float> cursor);

    void dragValue             (const juce::MouseEvent& e);
    void dragDanglingTip       (const juce::MouseEvent& e);
    void dragFlagConnection    (const juce::MouseEvent& e, Node& node, const NodePosition& newPosition);
    void endDrag ();

    void handleCanvasMouseDrag (const juce::MouseEvent& e);
    void handleNodeMouseDrag   (const juce::MouseEvent& e, Node& node);
    void handleNodeMouseDown   (const juce::MouseEvent& e, Node& node);
    void handleCanvasMouseDown (const juce::MouseEvent& e);

    void beginBoxSelection   (const juce::Point<int>& clickPoint);
    void updateBoxSelection  (const juce::MouseEvent& e);
    void setDraggedNodeVisible    (bool shouldBeVisible);

    void connectDanglingToTarget  (const Node* startNode);
    void connectWithSnapAnimation (int parentNodeId, int childNodeId, ArrowType rootConnectionType);
    void connectDraggedNodeToRoot ();

    void finishFlagConnection        ();
    void finishDanglingArrowCreation ();
    void finishArrowHeadDrag         ();
    void finishBoxSelection          ();
    void finishDanglingTipDrag       ();

    void selectSpanNode        (Node& node);
    void showSelectionMenu(juce::Point<int> canvasPoint);

    bool isNodeCreationModeActive     () const;
    bool isArrowMode                  () const { return arrowMode; }
    bool toggleEncapsulationExpansion (Node& node);


private:

    static constexpr float rootSnapThreshold       = 20.0f;
    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float flagProximityRadius     = 28.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;
    static constexpr float arrowLabelGrabRadius    = 10.0f;
    static constexpr int   dragThreshold           = 5;

    const ApplicationContext& applicationContext;
    NodeCanvas&               canvas;
    PopupWindowLauncher       allowedTraversalsLauncher { "Allowed Traversals" };

    ConnectionOps connectionOps { applicationContext };

    DragState dragState = DragState::Idle;

    juce::Component::SafePointer<Arrow> draggingDanglingArrow;
    juce::Component::SafePointer<Node>  danglingSnapTarget;
    juce::Component::SafePointer<Node>  danglingSourceNode;
    juce::Component::SafePointer<Node>  snapTargetRoot;
    juce::Component::SafePointer<Node>  draggingArrowHeadNode;
    juce::Component::SafePointer<Node>  draggingValueNode;
    juce::Component::SafePointer<Node> flagConnectionTarget;

    double dragStartValue = 0.0;

    juce::Point<float> dragParentCenter;
    juce::Point<int>  selectionAnchor;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;
    int flagConnectionSourceId   = -1;

    bool isDragStart = true;
    bool arrowMode   = false;
};
