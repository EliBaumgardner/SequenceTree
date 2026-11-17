#include "EncapsulationGestures.h"
#include "../../Graph/TypedProperties.h"
#include "../../UI/Canvas/NodeCanvas.h"
#include "../../UI/Node/Node.h"
#include "../../UI/Node/Encapsulator.h"
#include "../../Graph/NodeFactory.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

EncapsulationGestures::EncapsulationGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager)
    : nodeCanvas (nodeCanvas),
      graphState (graphState),
      undoManager(undoManager)
{
}

void EncapsulationGestures::selectSpanNode(Node& node)
{
    const int nodeId = node.nodeId;

    if (node.nodeType == NodeInfo::Type::Encapsulator) {
        nodeCanvas.clearOutlines();

        spanAnchorNodeId = -1;

        undoManager.beginNewTransaction();

        graphState.encapsulation.dissolve(nodeId, &undoManager);
        return;
    }

    if (node.nodeType != NodeInfo::Type::Node && node.nodeType != NodeInfo::Type::Root) {
        return;
    }

    const NodeProperties clicked(node.nodeValueTree);
    const NodeProperties anchor(graphState.getNode(spanAnchorNodeId));

    const int nodeRootId   = clicked.rootNodeId;
    const int anchorRootId = anchor.rootNodeId;

    if (spanAnchorNodeId < 0 || anchorRootId != nodeRootId) {
        nodeCanvas.clearOutlines();

        spanAnchorNodeId = nodeId;
        node.isOutlined  = true;

        node.repaint();

        return;
    }

    const std::vector<int> spanNodeIds = graphState.nodeIdsBetween(spanAnchorNodeId, nodeId);

    spanAnchorNodeId = -1;

    if (spanNodeIds.empty()) {
        return;
    }

    nodeCanvas.clearOutlines();

    undoManager.beginNewTransaction();

    NodeFactory(graphState, &undoManager).createEncapsulator(spanNodeIds);
}

void EncapsulationGestures::adoptCreatedChild(int parentNodeId, const juce::ValueTree& child)
{
    const NodeProperties parent(graphState.getNode(parentNodeId));

    const int owningEncapsulatorId = parent.encapsulatorId;

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded || owningEncapsulator->memberNodeIds.empty()) {
        return;
    }

    if (parentNodeId == owningEncapsulator->memberNodeIds.back()) {
        return;
    }

    graphState.encapsulation.insertNodeAfter(child.getProperty(ArsNovaUI::CanvasIdentifiers::Id), parentNodeId, &undoManager);
}

bool EncapsulationGestures::toggleEncapsulationExpansion(Node& node)
{
    const int nodeId = node.nodeId;

    if (node.nodeType == NodeInfo::Type::Encapsulator) {
        nodeCanvas.encapsulationView.expand(nodeId);
        return true;
    }

    const NodeProperties properties(node.nodeValueTree);

    const int owningEncapsulatorId = properties.encapsulatorId;

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(owningEncapsulatorId));

    if (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded || owningEncapsulator->memberNodeIds.empty()) {
        return false;
    }

    const bool isSpanBoundary = nodeId == owningEncapsulator->memberNodeIds.front()
                             || nodeId == owningEncapsulator->memberNodeIds.back();

    if (! isSpanBoundary) {
        return false;
    }

    nodeCanvas.encapsulationView.collapse(owningEncapsulatorId);
    return true;
}
