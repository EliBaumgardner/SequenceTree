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

    explicit NodeController(ApplicationContext& context);
    ~NodeController() override;

    void mouseEnter          (const juce::MouseEvent& e) override;
    void mouseExit           (const juce::MouseEvent& e) override;
    void mouseMove           (const juce::MouseEvent& e) override;
    void mouseDrag           (const juce::MouseEvent& e) override;
    void mouseUp             (const juce::MouseEvent& e) override;
    void mouseDown           (const juce::MouseEvent& e) override;

    void snapToGrid          (juce::UndoManager *undoManager, NodePosition &newPosition, juce::ValueTree draggedNodeTree);
    juce::Point<int> snapPointToGrid (juce::Point<int> point) const;

    void handleNodeDrag      (juce::UndoManager *undoManager, int nodeId, NodePosition newPosition);
    void handleNodeDragStart (juce::UndoManager *undoManager, Node *node, int nodeId, NodePosition newPosition, const juce::ModifierKeys& mods);

    void updateConnectionPreview (Node *node, const NodePosition& newPosition, bool dashed);

    void checkRootNodeSnap   (const NodePosition& pos);

    void  commitFlagConnection (int sourceNodeId, Node* target);

    void   showArrowContextMenu (Arrow* arrow);

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

    void handleCanvasMouseDown (const juce::MouseEvent& e, NodeCanvas& canvas);
    void handleNodeMouseDown   (const juce::MouseEvent& e, Node& node);

    void handleCanvasMouseDrag (const juce::MouseEvent& e, NodeCanvas& canvas);
    void handleNodeMouseDrag   (const juce::MouseEvent& e, Node& node);

    void dragValue         (const juce::MouseEvent& e);
    void dragDanglingTip   (const juce::MouseEvent& e, NodeCanvas& canvas);
    void dragFlagConnection(const juce::MouseEvent& e, Node& node, const NodePosition& newPosition);

    bool isNodeCreationModeActive (const NodeCanvas& canvas) const;

    void clearNodeSelection (NodeCanvas& canvas);
    void beginBoxSelection  (const juce::Point<int>& clickPoint, NodeCanvas& canvas);
    void updateBoxSelection (const juce::MouseEvent& e, NodeCanvas& canvas);
    void finishBoxSelection (NodeCanvas& canvas);

    void showSelectionMenu(juce::Point<int> canvasPoint);

    void finishArrowHeadDrag         (NodeCanvas& canvas);
    void finishDanglingTipDrag       (NodeCanvas& canvas);
    void finishFlagConnection        (NodeCanvas& canvas);
    void finishDanglingArrowCreation (NodeCanvas& canvas);
    void connectDraggedNodeToRoot    (NodeCanvas& canvas);

    void connectWithSnapAnimation (int parentNodeId, int childNodeId);
    void setDraggedNodeVisible    (bool shouldBeVisible);

    void endDrag  (NodeCanvas& canvas);
    void hideGrid (NodeCanvas& canvas) const;
    void showGrid (NodeCanvas& canvas) const;

    static constexpr float rootSnapThreshold       = 60.0f;
    static constexpr float danglingArrowGrabRadius = 14.0f;
    static constexpr float arrowHoverRadius        = 8.0f;
    static constexpr float arrowHeadGrabRadius     = 16.0f;
    static constexpr int   dragThreshold           = 5;
    static constexpr int   defaultNodeRadius       = 20;

    ApplicationContext& applicationContext;

    ConnectionOps connectionOps { applicationContext };
    SelectionOps  selectionOps  { applicationContext };

    DragState dragState = DragState::Idle;

    Arrow* draggingDanglingArrow = nullptr;

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
