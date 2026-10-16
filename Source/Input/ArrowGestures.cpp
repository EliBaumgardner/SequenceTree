#include "ArrowGestures.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../UI/Node/Encapsulator.h"
#include "../UI/Menus/AllowedTraversalsMenu.h"
#include "../Graph/NodeFactory.h"
#include "../Graph/GraphState.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

ArrowGestures::ArrowGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas),
      graphState(graphState),
      undoManager(undoManager)
{
}

void ArrowGestures::beginMovingTip(Arrow& danglingArrow)
{
    draggingDanglingArrow = &danglingArrow;
    tipGesture            = TipGesture::MovingDanglingTip;

    nodeCanvas.showGrid();
}

void ArrowGestures::dragTip(juce::Point<int> cursor)
{
    Node* sourceNode = newArrowSourceNode;

    if (tipGesture == TipGesture::MovingDanglingTip && draggingDanglingArrow == nullptr) {
        return;
    }

    if (tipGesture == TipGesture::MovingDanglingTip) {
        sourceNode = draggingDanglingArrow->startNode;
    }

    if (sourceNode == nullptr) {
        return;
    }

    tipSnapTarget = findTipSnapTarget(*sourceNode, cursor);

    juce::Point <int> tip = nodeCanvas.snapPointToGrid(cursor);
    const juce::Point<int> centre = sourceNode->getNodeCentre();

    if (tipSnapTarget != nullptr) {
        tip = tipSnapTarget->getNodeCentre();
    }

    if (tipGesture == TipGesture::MovingDanglingTip) {
        draggingDanglingArrow->dragging = true;

        draggingDanglingArrow->setTipOffset({ tip.x - centre.x, tip.y - centre.y });
        return;
    }

    if (tipGesture == TipGesture::CreatingDanglingArrow) {
        nodeCanvas.showGrid();
    }

    nodeCanvas.arrowManager.updatePreview(sourceNode, { tip.x - centre.x, tip.y - centre.y });
}

void ArrowGestures::finishTip()
{
    const TipGesture finishedGesture = tipGesture;
    Node* const      targetNode      = tipSnapTarget;
    Arrow*           draggedArrow    = nodeCanvas.arrowManager.preview.get();

    if (finishedGesture == TipGesture::MovingDanglingTip) {
        draggedArrow = draggingDanglingArrow;
    }

    tipGesture            = TipGesture::None;
    draggingDanglingArrow = nullptr;
    newArrowSourceNode    = nullptr;
    tipSnapTarget         = nullptr;

    nodeCanvas.hideGrid();

    if (draggedArrow == nullptr) {
        return;
    }

    Node* const sourceNode = draggedArrow->startNode;

    draggedArrow->endDrag();

    if (targetNode == nullptr && finishedGesture == TipGesture::MovingDanglingTip) {
        undoManager.beginNewTransaction();

        NodeFactory::setDanglingArrowTip(graphState, draggedArrow->arrowTree, draggedArrow->tipOffset, &undoManager);
        return;
    }

    if (targetNode == nullptr && finishedGesture != TipGesture::ConnectingFlag) {
        undoManager.beginNewTransaction();

        nodeCanvas.arrowManager.commitPreview();
        return;
    }

    nodeCanvas.arrowManager.preview.reset();

    if (targetNode != nullptr && sourceNode != nullptr && sourceNode->nodeArrows.count(targetNode->nodeId) == 0) {
        undoManager.beginNewTransaction();

        graphState.connectNodes(sourceNode->nodeId, targetNode->nodeId, &undoManager);

        NodeFactory::applyArrowInfo(graphState, sourceNode->nodeId, targetNode->nodeId, nodeCanvas.arrowManager.currentArrowInfo,
                                    ArrowType::StepIntoTree, &undoManager);

        nodeCanvas.arrowManager.connectWithSnapAnimation(sourceNode->nodeId, targetNode->nodeId);
    }

    if (targetNode != nullptr && finishedGesture == TipGesture::MovingDanglingTip) {
        NodeFactory::destroyDanglingArrow(draggedArrow->arrowTree, &undoManager);
    }
}

void ArrowGestures::updateHover(juce::Point<float> cursor)
{
    Arrow* const hoveredArrow = nodeCanvas.hitTester.arrowNear(cursor, arrowHoverRadius);

    for (Arrow* arrow : nodeCanvas.arrowManager.all()) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }

        if (arrow->startNode->nodeType != NodeType::TraversalFlag) {
            continue;
        }

        const float dist   = CanvasHitTester::distanceToSegment(cursor, arrow->startNode->getNodeCentre().toFloat(), arrow->endNode->getNodeCentre().toFloat());
        const bool  nearby = dist < flagProximityRadius;

        if (nearby != arrow->proximityHovered) {
            arrow->proximityHovered = nearby;

            arrow->setHoverFade(arrow->sourceHovered || arrow->proximityHovered);
        }
    }

    for (Arrow* arrow : nodeCanvas.arrowManager.all()) {
        const bool shouldBold = (arrow == hoveredArrow);

        if (shouldBold != arrow->hovered) {
            arrow->hovered = shouldBold;

            arrow->repaint();
        }
    }
}

void ArrowGestures::removeArrow(Arrow& arrow)
{
    if (arrow.isDangling()) {
        undoManager.beginNewTransaction();

        NodeFactory::destroyDanglingArrow(arrow.arrowTree, &undoManager);
        return;
    }

    if (arrow.startNode == nullptr || arrow.endNode == nullptr) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(&arrow);

    undoManager.beginNewTransaction();

    if (graphState.getNode(childNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData) {
        graphState.removeNode(childNodeId, &undoManager);
        return;
    }

    graphState.disconnectNodes(ownerNodeId, childNodeId, &undoManager);
}

void ArrowGestures::showArrowContextMenu(Arrow* arrow)
{
    ContextMenu menu;

    if (arrow == nullptr) {
        return;
    }

    menu.addItem("edit allowed traversals", ContextMenu::ItemKind::Action, [this, arrow]() {
        juce::ValueTree connection = connectionTreeFor(arrow);

        if (! connection.isValid()) {
            return;
        }

        allowedTraversalsLauncher.show(nodeCanvas, [this, connection]() {
            auto content = std::make_unique<AllowedTraversalsMenu>(CustomLookAndFeel::get(nodeCanvas), graphState, undoManager, connection);

            content->setSize(AllowedTraversalsMenu::defaultWidth, content->getIdealHeight());

            return content;
        });
    });

    menu.addItem("traversal arrow", ContextMenu::ItemKind::Toggle, [this, arrow]() {
        if (arrow->isTraversalArrow()) {
            setArrowType(arrow, ArrowType::Node);
            return;
        }

        setArrowType(arrow, ArrowType::Traversal);
    }, canBeTraversalArrow(arrow), arrow->isTraversalArrow());

    if (connectsToModulatorRoot(arrow)) {
        menu.addItem("sync", ContextMenu::ItemKind::Toggle, [this, arrow]() {
            setArrowSync(arrow, ! arrow->isSyncArrow());
        }, true, arrow->isSyncArrow());
    }

    menu.show(*arrow);
}

void ArrowGestures::setArrowMode(bool enabled)
{
    arrowMode = enabled;

    if (! enabled) {
        nodeCanvas.arrowManager.preview.reset();
    }
}

bool ArrowGestures::beginTipFromNode(Node& node)
{
    auto* const collapsedEncapsulator = dynamic_cast<Encapsulator*>(&node);

    if (node.nodeType == NodeType::TraversalFlag && ! arrowMode) {
        tipGesture         = TipGesture::ConnectingFlag;
        newArrowSourceNode = &node;
        return true;
    }

    if (! arrowMode) {
        return false;
    }

    tipGesture         = TipGesture::CreatingDanglingArrow;
    newArrowSourceNode = &node;

    if (collapsedEncapsulator != nullptr && ! collapsedEncapsulator->memberNodeIds.empty()) {
        Node* const exitMember = nodeCanvas.nodeManager.find(collapsedEncapsulator->memberNodeIds.back());

        if (exitMember != nullptr) {
            newArrowSourceNode = exitMember;
        }
    }

    return true;
}

Node* ArrowGestures::findTipSnapTarget(const Node& startNode, juce::Point<int> tip) const
{
    const int startNodeId = startNode.nodeId;

    if (tipGesture == TipGesture::ConnectingFlag) {
        return nodeCanvas.hitTester.nodeNear(tip.toFloat(), flagSnapThreshold, startNodeId);
    }

    Node* const snapTarget = nodeCanvas.hitTester.nodeContaining(tip.toFloat(), startNodeId);

    if (snapTarget == nullptr) {
        return nullptr;
    }

    if (startNode.nodeArrows.count(snapTarget->nodeId) > 0) {
        return nullptr;
    }

    const juce::Identifier startType = startNode.nodeValueTree.getType();

    const bool startsOnModulator = startType == ValueTreeIdentifiers::ModulatorData
                                || startType == ValueTreeIdentifiers::ModulatorRootData
                                || startType == ValueTreeIdentifiers::AlternativeModulatorData;

    if (startsOnModulator && snapTarget->nodeValueTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        return nullptr;
    }

    return snapTarget;
}

void ArrowGestures::setArrowType(const Arrow* arrow, ArrowType arrowType)
{
    if (! canBeTraversalArrow(arrow)) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    const juce::ValueTree connection = graphState.getConnection(ownerNodeId, childNodeId);

    ArrowInfo arrowInfo = ArrowBindingOps::getArrowInfo(connection);
    arrowInfo.type      = arrowType;

    undoManager.beginNewTransaction();
    ArrowBindingOps::setArrowInfo(connection, arrowInfo, &undoManager);
}

void ArrowGestures::setArrowSync(const Arrow* arrow, bool shouldSync)
{
    if (! connectsToModulatorRoot(arrow)) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    const juce::ValueTree connection = graphState.getConnection(ownerNodeId, childNodeId);

    ArrowInfo arrowInfo = ArrowBindingOps::getArrowInfo(connection);
    arrowInfo.isSynced  = shouldSync;

    undoManager.beginNewTransaction();
    ArrowBindingOps::setArrowInfo(connection, arrowInfo, &undoManager);
}

bool ArrowGestures::canBeTraversalArrow(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    return graphState.connectsToOtherTreeRoot(ownerNodeId, childNodeId);
}

bool ArrowGestures::connectsToModulatorRoot(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    const juce::ValueTree childTree = graphState.getNode(childNodeId);

    return childTree.getType() == ValueTreeIdentifiers::ModulatorRootData;
}

juce::ValueTree ArrowGestures::connectionTreeFor(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr) {
        return {};
    }

    if (arrow->isDangling()) {
        return arrow->arrowTree;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    juce::ValueTree ownerTree     = graphState.getNode(ownerNodeId);
    juce::ValueTree ownerChildren = ownerTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return ownerChildren.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}

ArrowGestures::ArrowOwnership ArrowGestures::resolveOwnership(const Arrow* arrow) const
{
    const int startId = arrow->startNode->nodeId;
    const int endId   = arrow->endNode->nodeId;

    juce::ValueTree startTree     = graphState.getNode(startId);
    juce::ValueTree startChildren = startTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
    bool startOwnsEnd = startChildren.getChildWithProperty(ValueTreeIdentifiers::Id, endId).isValid();

    if (startOwnsEnd) {
        return { startId, endId };
    }

    return { endId, startId };
}
