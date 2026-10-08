#include "ConnectionOps.h"
#include "../UI/Node/Arrow.h"
#include "../UI/Node/Node.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/GraphState.h"

void ConnectionOps::disconnect(const Arrow* arrow)
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    undoManager.beginNewTransaction();

    if (graphState.getNode(childNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData) {
        graphState.removeNode(childNodeId, &undoManager);
        return;
    }

    graphState.disconnectNodes(ownerNodeId, childNodeId, &undoManager);
}

ConnectionOps::ArrowOwnership ConnectionOps::resolveOwnership(const Arrow* arrow) const
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

juce::ValueTree ConnectionOps::connectionTreeFor(const Arrow* arrow) const
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

void ConnectionOps::connect(int parentNodeId, int childNodeId, ArrowType rootConnectionType)
{
    undoManager.beginNewTransaction();
    graphState.connectNodes(parentNodeId, childNodeId, &undoManager);
    applySelectedArrowInfo(parentNodeId, childNodeId, rootConnectionType);
}

void ConnectionOps::applySelectedArrowInfo(int parentNodeId, int childNodeId,
                                           ArrowType rootConnectionType)
{
    ArrowInfo arrowInfo = currentArrowInfo;

    const bool connectsToOtherRoot = connectsToOtherTreeRoot(parentNodeId, childNodeId);

    if (arrowInfo.type == ArrowType::Traversal && ! connectsToOtherRoot) {
        arrowInfo.type = ArrowType::Node;
    }

    const bool ownedByTraversalFlag = graphState.getNode(parentNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData;

    if (connectsToOtherRoot && ! ownedByTraversalFlag && arrowInfo.type != ArrowType::Traversal) {
        arrowInfo.type = rootConnectionType;
    }

    const juce::ValueTree connection = graphState.getConnection(parentNodeId, childNodeId);

    graphState.arrows.applyNodeBinding(arrowInfo, parentNodeId, connection);

    ArrowBindingOps::setArrowInfo(connection, arrowInfo, &undoManager);

    graphState.arrows.syncPitchBindings(childNodeId, &undoManager);
}

bool ConnectionOps::connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const
{
    const juce::ValueTree childTree = graphState.getNode(childNodeId);

    if (childTree.getType() != ValueTreeIdentifiers::RootNodeData) {
        return false;
    }

    const juce::ValueTree parentTree = graphState.getNode(parentNodeId);

    if (parentTree.getType() == ValueTreeIdentifiers::ModulatorData
        || parentTree.getType() == ValueTreeIdentifiers::ModulatorRootData
        || parentTree.getType() == ValueTreeIdentifiers::AlternativeModulatorData) {
        return false;
    }

    return static_cast<int>(parentTree.getProperty(ValueTreeIdentifiers::RootNodeId)) != childNodeId;
}

bool ConnectionOps::canBeTraversalArrow(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    return connectsToOtherTreeRoot(ownerNodeId, childNodeId);
}

void ConnectionOps::setArrowType(const Arrow* arrow, ArrowType arrowType)
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

bool ConnectionOps::connectsToModulatorRoot(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    const juce::ValueTree childTree = graphState.getNode(childNodeId);

    return childTree.getType() == ValueTreeIdentifiers::ModulatorRootData;
}

void ConnectionOps::setArrowSync(const Arrow* arrow, bool shouldSync)
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
