#include "EncapsulationOps.h"
#include "GraphState.h"
#include "ValueTreeIdentifiers.h"

#include <unordered_set>

juce::ValueTree EncapsulationOps::create(const std::vector<int>& memberNodeIds, juce::UndoManager* undoManager)
{
    jassert(! memberNodeIds.empty());

    const juce::ValueTree firstMember = graphState.getNode(memberNodeIds.front());

    jassert(firstMember.isValid());

    graphState.nodeIdIncrement = graphState.nodeIdIncrement + 1;

    const int encapsulatorId = graphState.nodeIdIncrement;

    juce::ValueTree encapsulator    {ValueTreeIdentifiers::EncapsulatorData};
    juce::ValueTree encapsulatedIds {ValueTreeIdentifiers::EncapsulatedIds};

    for (const int memberNodeId : memberNodeIds) {
        juce::ValueTree memberId {ValueTreeIdentifiers::NodeId};
        memberId.setProperty(ValueTreeIdentifiers::Id, memberNodeId, undoManager);

        encapsulatedIds.addChild(memberId, -1, undoManager);

        graphState.getNode(memberNodeId).setProperty(ValueTreeIdentifiers::EncapsulatorId, encapsulatorId, undoManager);
    }

    encapsulator.addChild(encapsulatedIds, -1, undoManager);

    encapsulator.setProperty(ValueTreeIdentifiers::RootNodeId,
                             firstMember.getProperty(ValueTreeIdentifiers::RootNodeId), undoManager);
    encapsulator.setProperty(ValueTreeIdentifiers::Id, encapsulatorId, undoManager);

    graphState.nodeMap.addChild(encapsulator, -1, undoManager);

    return encapsulator;
}

int EncapsulationOps::unusedLabel() const
{
    std::unordered_set<int> usedLabels;

    for (int i = 0; i < graphState.nodeMap.getNumChildren(); ++i) {
        const juce::ValueTree existing = graphState.nodeMap.getChild(i);

        if (existing.getType() == ValueTreeIdentifiers::EncapsulatorData) {
            usedLabels.insert((int) existing.getProperty(ValueTreeIdentifiers::EncapsulatorLabel));
        }
    }

    int encapsulatorLabel = 0;

    while (usedLabels.count(encapsulatorLabel) > 0) {
        encapsulatorLabel = encapsulatorLabel + 1;
    }

    return encapsulatorLabel;
}

std::vector<int> EncapsulationOps::memberIds(int encapsulatorId) const
{
    const juce::ValueTree encapsulatedIds =
        graphState.getNode(encapsulatorId).getChildWithName(ValueTreeIdentifiers::EncapsulatedIds);

    std::vector<int> memberNodeIds;

    for (int i = 0; i < encapsulatedIds.getNumChildren(); ++i) {
        memberNodeIds.push_back(encapsulatedIds.getChild(i).getProperty(ValueTreeIdentifiers::Id));
    }

    return memberNodeIds;
}

void EncapsulationOps::insertNodeAfter(int nodeId, int siblingNodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree node = graphState.getNode(nodeId);

    const int encapsulatorId = graphState.getNode(siblingNodeId).getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    juce::ValueTree encapsulatedIds =
        graphState.getNode(encapsulatorId).getChildWithName(ValueTreeIdentifiers::EncapsulatedIds);

    if (! node.isValid() || ! encapsulatedIds.isValid()) {
        return;
    }

    const juce::ValueTree siblingId = encapsulatedIds.getChildWithProperty(ValueTreeIdentifiers::Id, siblingNodeId);

    if (! siblingId.isValid() || encapsulatedIds.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId).isValid()) {
        return;
    }

    juce::ValueTree memberId {ValueTreeIdentifiers::NodeId};
    memberId.setProperty(ValueTreeIdentifiers::Id, nodeId, undoManager);

    encapsulatedIds.addChild(memberId, encapsulatedIds.indexOf(siblingId) + 1, undoManager);

    node.setProperty(ValueTreeIdentifiers::EncapsulatorId, encapsulatorId, undoManager);
}

void EncapsulationOps::dissolve(int encapsulatorId, juce::UndoManager* undoManager)
{
    juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

    if (! encapsulator.isValid()) {
        return;
    }

    for (const int memberNodeId : memberIds(encapsulatorId)) {
        juce::ValueTree member = graphState.getNode(memberNodeId);

        if (member.isValid()) {
            member.removeProperty(ValueTreeIdentifiers::EncapsulatorId, undoManager);
        }
    }

    graphState.nodeMap.removeChild(encapsulator, undoManager);
}

void EncapsulationOps::removeGroup(int encapsulatorId, juce::UndoManager* undoManager)
{
    for (const int memberNodeId : memberIds(encapsulatorId)) {
        if (graphState.getNode(memberNodeId).isValid()) {
            graphState.removeNode(memberNodeId, undoManager);
        }
    }

    dissolve(encapsulatorId, undoManager);
}
