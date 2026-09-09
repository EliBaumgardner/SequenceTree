/*
  ==============================================================================

    ObjectObjectController
    Created: 6 May 2025 8:38:35pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once
#include "../Util/PluginModules.h"
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
    NodeControllerMode nodeControllerMode;

    NodeController(ApplicationContext& context, NodeCanvas& canvas);
    ~NodeController() override;

    void mouseEnter          (const juce::MouseEvent& e) override;
    void mouseExit           (const juce::MouseEvent& e) override;
    void mouseMove           (const juce::MouseEvent& e) override;
    void mouseDrag           (const juce::MouseEvent& e) override;
    void mouseUp             (const juce::MouseEvent& e) override;
    void mouseDown           (const juce::MouseEvent& e) override;

    void snapToGrid          (juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree);

    void handleNodeDrag      (juce::UndoManager *undoManager, int nodeId, NodePosition newPosition);
    void handleNodeDragStart (juce::UndoManager *undoManager, Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods);

    void updateConnectionPreview (Node *node, const NodePosition& newPosition, bool dashed);

    void checkRootNodeSnap   (juce::Point<int> canvasPoint);

    void  commitFlagConnection (int sourceNodeId, Node* target);

    void   showArrowContextMenu (Arrow* arrow);

    void setArrowMode (bool enabled);

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

    enum SelectionMenuItem {
        copy = 1,
        paste,
        remove
    };

    enum ArrowMenuItem {
        editAllowedTraversals = 1,
        traversalArrow
    };

    void handleCanvasMouseDown (const juce::MouseEvent& e);
    void handleNodeMouseDown   (const juce::MouseEvent& e, Node& node);

    void selectSpanNode        (Node& node);

    bool toggleEncapsulationExpansion (Node& node);

    void handleCanvasMouseDrag (const juce::MouseEvent& e);
    void handleNodeMouseDrag   (const juce::MouseEvent& e, Node& node);

    void updateArrowHover  (juce::Point<float> cursor);

    void dragValue         (const juce::MouseEvent& e);
    void dragDanglingTip   (const juce::MouseEvent& e);
    void dragFlagConnection(const juce::MouseEvent& e, Node& node, const NodePosition& newPosition);

    bool isArrowMode () const { return arrowMode; }
    bool isNodeCreationModeActive () const;

    void beginBoxSelection  (const juce::Point<int>& clickPoint);
    void updateBoxSelection (const juce::MouseEvent& e);
    void finishBoxSelection ();

    void showSelectionMenu(juce::Point<int> canvasPoint);

    void finishArrowHeadDrag         ();
    void finishDanglingTipDrag       ();
    void finishFlagConnection        ();
    void finishDanglingArrowCreation ();
    void connectDraggedNodeToRoot    ();

    Node* findDanglingSnapTarget   (const Node* startNode, juce::Point<int> tip) const;
    juce::Point<int> danglingTipFor (const Node* startNode, juce::Point<int> cursor);
    void  connectDanglingToTarget  (const Node* startNode);

    void connectWithSnapAnimation (int parentNodeId, int childNodeId);
    void setDraggedNodeVisible    (bool shouldBeVisible);

    void endDrag  ();

    static constexpr float rootSnapThreshold       = 60.0f;
    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float flagProximityRadius     = 28.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;
    static constexpr int   dragThreshold           = 5;
    static constexpr int   defaultNodeRadius       = 20;

    ApplicationContext& applicationContext;
    NodeCanvas&         canvas;

    ConnectionOps connectionOps { applicationContext };
    SelectionOps  selectionOps  { applicationContext };

    DragState dragState = DragState::Idle;

    bool arrowMode = false;

    Arrow* draggingDanglingArrow = nullptr;

    Node* danglingSnapTarget = nullptr;

    PopupWindowLauncher allowedTraversalsLauncher { "Allowed Traversals" };

    Node* snapTargetRoot           = nullptr;

    Node* draggingArrowHeadNode    = nullptr;

    int   flagConnectionSourceId   = -1;
    Node* flagConnectionTarget     = nullptr;

    bool isDragStart               = true;

    double dragStartValue          = 0.0;
    Node*  draggingValueNode       = nullptr;

    juce::Point<float> dragParentCenter;

    juce::Point<int> selectionAnchor;

    juce::ValueTree draggedNodeTree;

    int snapSourceNodeId = -1;
};
