#include "ConnectionOps.h"
#include "../UI/Node/Arrow.h"
#include "../UI/Node/Node.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "../Graph/ValueTreeState.h"
#include "../Graph/RTGraphBuilder.h"

ConnectionOps::ArrowOwnership ConnectionOps::resolveOwnership(const Arrow* arrow) const
{
    const int startId = arrow->startNode->getComponentID().getIntValue();
    const int endId   = arrow->endNode->getComponentID().getIntValue();

    juce::ValueTree startTree     = applicationContext.valueTreeState->getNode(startId);
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
    applicationContext.valueTreeState->disconnectNodes(ownerNodeId, childNodeId, undoManager);
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

    juce::ValueTree ownerTree     = applicationContext.valueTreeState->getNode(ownerNodeId);
    juce::ValueTree ownerChildren = ownerTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return ownerChildren.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}

bool ConnectionOps::connectsToOtherTreeRoot(int parentNodeId, int childNodeId) const
{
    const juce::ValueTree childTree = applicationContext.valueTreeState->getNode(childNodeId);

    if (childTree.getType() != ValueTreeIdentifiers::RootNodeData) {
        return false;
    }

    const juce::ValueTree parentTree = applicationContext.valueTreeState->getNode(parentNodeId);

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

    undoManager->beginNewTransaction();
    applicationContext.valueTreeState->setArrowType(ownerNodeId, childNodeId, arrowType, undoManager);
}

void ConnectionOps::applySelectedArrowType(int parentNodeId, int childNodeId)
{
    if (applicationContext.currentArrowType != ArrowType::Traversal) {
        return;
    }

    if (! connectsToOtherTreeRoot(parentNodeId, childNodeId)) {
        return;
    }

    applicationContext.valueTreeState->setArrowType(parentNodeId, childNodeId, ArrowType::Traversal,
                                                    applicationContext.undoManager);
}

void ConnectionOps::connect(int parentNodeId, int childNodeId)
{
    juce::UndoManager* undoManager = applicationContext.undoManager;

    undoManager->beginNewTransaction();
    applicationContext.valueTreeState->connectNodes(parentNodeId, childNodeId, undoManager);
    applySelectedArrowType(parentNodeId, childNodeId);

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.valueTreeState->getNode(parentNodeId));
}
