#include "ConnectionOps.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Arrow.h"
#include "../UI/Node/Node.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/GraphState.h"
#include "../Graph/RTGraphBuilder.h"

ConnectionOps::ArrowOwnership ConnectionOps::resolveOwnership(const Arrow* arrow) const
{
    const int startId = arrow->startNode->getComponentID().getIntValue();
    const int endId   = arrow->endNode->getComponentID().getIntValue();

    juce::ValueTree startTree     = applicationContext.graphState->getNode(startId);
    juce::ValueTree startChildren = startTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
    bool startOwnsEnd = startChildren.getChildWithProperty(ValueTreeIdentifiers::Id, endId).isValid();

    if (startOwnsEnd) {
        return { startId, endId };
    }

    return { endId, startId };
}

void ConnectionOps::disconnect(const Arrow* arrow)
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    juce::UndoManager* undoManager = applicationContext.undoManager;
    undoManager->beginNewTransaction();
    applicationContext.graphState->disconnectNodes(ownerNodeId, childNodeId, undoManager);
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

    juce::ValueTree ownerTree     = applicationContext.graphState->getNode(ownerNodeId);
    juce::ValueTree ownerChildren = ownerTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return ownerChildren.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}

bool ConnectionOps::connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const
{
    const juce::ValueTree childTree = applicationContext.graphState->getNode(childNodeId);

    if (childTree.getType() != ValueTreeIdentifiers::RootNodeData) {
        return false;
    }

    const juce::ValueTree parentTree = applicationContext.graphState->getNode(parentNodeId);

    return (int) parentTree.getProperty(ValueTreeIdentifiers::RootNodeId) != childNodeId;
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

    juce::UndoManager* undoManager = applicationContext.undoManager;

    GraphState& state = *applicationContext.graphState;

    const juce::ValueTree connection = state.getConnection(ownerNodeId, childNodeId);

    ArrowInfo arrowInfo = ArrowBindingOps::getArrowInfo(connection);
    arrowInfo.type      = arrowType;

    undoManager->beginNewTransaction();
    ArrowBindingOps::setArrowInfo(connection, arrowInfo, undoManager);
}

bool ConnectionOps::connectsToModulatorRoot(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    const juce::ValueTree childTree = applicationContext.graphState->getNode(childNodeId);

    return childTree.getType() == ValueTreeIdentifiers::ModulatorRootData;
}

void ConnectionOps::setArrowSync(const Arrow* arrow, bool shouldSync)
{
    if (! connectsToModulatorRoot(arrow)) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = resolveOwnership(arrow);

    juce::UndoManager* undoManager = applicationContext.undoManager;

    GraphState& state = *applicationContext.graphState;

    const juce::ValueTree connection = state.getConnection(ownerNodeId, childNodeId);

    ArrowInfo arrowInfo = ArrowBindingOps::getArrowInfo(connection);
    arrowInfo.isSynced  = shouldSync;

    undoManager->beginNewTransaction();
    ArrowBindingOps::setArrowInfo(connection, arrowInfo, undoManager);
}

void ConnectionOps::applySelectedArrowInfo(int parentNodeId, int childNodeId,
                                           ArrowType rootConnectionType)
{
    GraphState& state = *applicationContext.graphState;

    ArrowInfo arrowInfo = applicationContext.canvas->arrowManager.currentArrowInfo;

    const bool connectsToOtherRoot = connectsToOtherTreeRoot(parentNodeId, childNodeId);

    if (arrowInfo.type == ArrowType::Traversal && ! connectsToOtherRoot) {
        arrowInfo.type = ArrowType::Node;
    }

    const bool ownedByTraversalFlag = state.getNode(parentNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData;

    if (connectsToOtherRoot && ! ownedByTraversalFlag && arrowInfo.type != ArrowType::Traversal) {
        arrowInfo.type = rootConnectionType;
    }

    const juce::ValueTree connection = state.getConnection(parentNodeId, childNodeId);

    state.arrows.applyNodeBinding(arrowInfo, parentNodeId, connection);

    ArrowBindingOps::setArrowInfo(connection, arrowInfo, applicationContext.undoManager);

    for (const int repitchedNodeId : applicationContext.graphState->arrows.syncPitchBindings(childNodeId,
                                                                                         applicationContext.undoManager)) {
        applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.graphState->getNode(repitchedNodeId));
    }
}

void ConnectionOps::connect(int parentNodeId, int childNodeId, ArrowType rootConnectionType)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;

    undoManager->beginNewTransaction();
    applicationContext.graphState->connectNodes(parentNodeId, childNodeId, undoManager);
    applySelectedArrowInfo(parentNodeId, childNodeId, rootConnectionType);

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.graphState->getNode(parentNodeId));
}
