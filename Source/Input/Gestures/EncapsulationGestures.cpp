#include "EncapsulationGestures.h"
#include "../../UI/Canvas/NodeCanvas.h"
#include "../../UI/Node/Node.h"
#include "../../UI/Node/Encapsulator.h"
#include "../../Graph/NodeFactory.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

EncapsulationGestures::EncapsulationGestures(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas),
      graphState(graphState),
      undoManager(undoManager)
{
}

void EncapsulationGestures::selectSpanNode(Node& node)
{
    const int nodeId = node.nodeId;

    if (node.nodeType == NodeInfo::Type::Encapsulator) {
        nodeCanvas.clearOutlines();
        nodeCanvas.spanAnchorNodeId = -1;

        undoManager.beginNewTransaction();

        graphState.encapsulation.dissolve(nodeId, &undoManager);
        return;
    }

    if (node.nodeType != NodeInfo::Type::Node && node.nodeType != NodeInfo::Type::Root) {
        return;
    }

    const int nodeRootId   = node.nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    const int anchorRootId = graphState.getNode(nodeCanvas.spanAnchorNodeId).getProperty(ValueTreeIdentifiers::RootNodeId);

    if (nodeCanvas.spanAnchorNodeId < 0 || anchorRootId != nodeRootId) {
        nodeCanvas.clearOutlines();

        nodeCanvas.spanAnchorNodeId = nodeId;

        node.isOutlined = true;
        node.repaint();
        return;
    }

    const std::vector<int> spanNodeIds = graphState.nodeIdsBetween(nodeCanvas.spanAnchorNodeId, nodeId);

    nodeCanvas.spanAnchorNodeId = -1;

    if (spanNodeIds.empty()) {
        return;
    }

    nodeCanvas.clearOutlines();

    undoManager.beginNewTransaction();

    NodeFactory::createEncapsulator(graphState, spanNodeIds, &undoManager);
}

void EncapsulationGestures::adoptCreatedChild(int parentNodeId, const juce::ValueTree& child)
{
    const int owningEncapsulatorId = graphState.getNode(parentNodeId).getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

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

    const int owningEncapsulatorId = node.nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

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
