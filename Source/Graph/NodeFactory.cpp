#include "NodeFactory.h"
#include "ValueTreeIdentifiers.h"

void NodeFactory::createRootNode(GraphState& state, const NodePosition& nodePosition, juce::UndoManager* undoManager)
{
    const juce::ValueTree rootNodeValueTree = state.addRootNode(undoManager);
    const int             rootId            = rootNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    state.setNodePosition(rootNodeValueTree, nodePosition, undoManager);

    setDefaultNodeNote(state, rootId, undoManager);
    setDefaultTraversal(state, rootId, undoManager);
}

juce::ValueTree NodeFactory::createNode(GraphState& state, int parentNodeId, const juce::Identifier& nodeType,
                                        const NodePosition& nodePosition, juce::UndoManager* undoManager)
{
    const juce::ValueTree childNodeValueTree = state.addChildNode(parentNodeId, nodeType, undoManager);
    const int             nodeId             = childNodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    state.setNodePosition(childNodeValueTree, nodePosition, undoManager);

    inheritFromParent(state, parentNodeId, nodeId, childNodeValueTree, undoManager);

    return childNodeValueTree;
}

juce::ValueTree NodeFactory::createTraversalFlagNode(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                                     juce::UndoManager* undoManager)
{
    const juce::ValueTree childNodeValueTree = state.addTraversalFlagNode(parentNodeId, undoManager);

    state.setNodePosition(childNodeValueTree, nodePosition, undoManager);

    return childNodeValueTree;
}

juce::ValueTree NodeFactory::createModulator(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                             juce::UndoManager* undoManager)
{
    const juce::ValueTree modulatorValueTree = state.addModulator(parentNodeId, undoManager);

    state.setNodePosition(modulatorValueTree, nodePosition, undoManager);

    return modulatorValueTree;
}

juce::ValueTree NodeFactory::createAlternativeModulator(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                                        juce::UndoManager* undoManager)
{
    const juce::ValueTree alternativeModulatorValueTree = state.addAlternativeModulator(parentNodeId, undoManager);

    state.setNodePosition(alternativeModulatorValueTree, nodePosition, undoManager);

    return alternativeModulatorValueTree;
}

juce::ValueTree NodeFactory::createModulatorRoot(GraphState& state, int parentNodeId, const NodePosition& nodePosition,
                                                 juce::UndoManager* undoManager)
{
    const juce::ValueTree modulatorRootValueTree = state.addModulatorRoot(parentNodeId, undoManager);

    state.setNodePosition(modulatorRootValueTree, nodePosition, undoManager);

    return modulatorRootValueTree;
}

juce::ValueTree NodeFactory::createEncapsulator(GraphState& state, std::span<const int> memberNodeIds,
                                                int subLoopCountLimit, juce::UndoManager* undoManager)
{
    const int       encapsulatorLabel     = state.encapsulation.unusedLabel();
    juce::ValueTree encapsulatorValueTree = state.encapsulation.create(memberNodeIds, undoManager);

    encapsulatorValueTree.setProperty(ValueTreeIdentifiers::EncapsulatorLabel, encapsulatorLabel, undoManager);
    encapsulatorValueTree.setProperty(ValueTreeIdentifiers::SubLoopCountLimit, subLoopCountLimit, undoManager);

    state.setNodePosition(encapsulatorValueTree, state.getNodePosition(memberNodeIds.front()), undoManager);

    return encapsulatorValueTree;
}

void NodeFactory::createDanglingArrow(GraphState& state, juce::ValueTree nodeTree, const juce::Point<int>& tipOffset,
                                      const ArrowInfo& arrowInfo, juce::UndoManager* undoManager)
{
    if (!nodeTree.isValid()) {
        return;
    }

    juce::ValueTree arrowList = nodeTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);
    juce::ValueTree arrowTree(ValueTreeIdentifiers::DanglingArrow);

    if (!arrowList.isValid()) {
        arrowList = juce::ValueTree(ValueTreeIdentifiers::DanglingArrows);

        nodeTree.addChild(arrowList, -1, undoManager);
    }

    arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::CountLimit, GraphState::defaultNodeCountLimit, undoManager);

    ArrowBindingOps::setArrowInfo(arrowTree, arrowInfo, undoManager);

    arrowList.addChild(arrowTree, -1, undoManager);

    state.arrows.syncPitchBindings(nodeTree.getProperty(ValueTreeIdentifiers::Id), undoManager);
}

void NodeFactory::destroyDanglingArrow(juce::ValueTree arrowTree, juce::UndoManager* undoManager)
{
    juce::ValueTree arrowList = arrowTree.getParent();

    if (arrowList.isValid()) {
        arrowList.removeChild(arrowTree, undoManager);
    }
}

void NodeFactory::setDanglingArrowTip(GraphState& state, juce::ValueTree arrowTree, const juce::Point<int>& tipOffset,
                                      juce::UndoManager* undoManager)
{
    if (!arrowTree.isValid()) {
        return;
    }

    arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration, ArrowInfo::noDurationOverride, undoManager);

    state.arrows.syncPitchBindings(arrowTree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id), undoManager);
}

void NodeFactory::setDefaultTraversal(GraphState& state, int nodeId, juce::UndoManager* undoManager)
{
    const juce::ValueTree rootNodeValueTree    = state.getNode(nodeId);
    const int             traversalInstance    = state.traversals.unusedInstance(TraversalState::defaultTraversalId);
    juce::ValueTree       traversalChildrenIds = rootNodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
    juce::ValueTree       traversalId          { ValueTreeIdentifiers::TraversalId };

    state.traversals.addTraversalData(TraversalState::defaultTraversalId, undoManager);

    traversalId.setProperty(ValueTreeIdentifiers::TraversalId,       TraversalState::defaultTraversalId, undoManager);
    traversalId.setProperty(ValueTreeIdentifiers::TraversalInstance, traversalInstance,                  undoManager);

    traversalChildrenIds.addChild(traversalId, -1, undoManager);
}

void NodeFactory::setDefaultNodeNote(GraphState& state, int nodeId, juce::UndoManager* undoManager)
{
    NodeNote note;

    note.pitch       = 60;
    note.velocity    = 60;
    note.duration    = 1000;
    note.midiChannel = GraphState::defaultMidiChannel;

    state.addMidiNote(nodeId, note, undoManager);
}

void NodeFactory::inheritFromParent(GraphState& state, int parentNodeId, int newNodeId, juce::ValueTree newNode,
                                    juce::UndoManager* undoManager)
{
    const juce::ValueTree  parentNode         = state.getNode(parentNodeId);
    const juce::ValueTree  parentMidi         = parentNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
    juce::ValueTree        newMidi            = newNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
    const juce::Identifier inheritedProperties[] = {
        ValueTreeIdentifiers::CountLimit,
        ValueTreeIdentifiers::SwitchCountLimit,
        ValueTreeIdentifiers::SubLoopCountLimit,
        ValueTreeIdentifiers::RepeatValue,
        ValueTreeIdentifiers::Probability
    };

    const bool parentIsNoteNode = parentNode.getType() == ValueTreeIdentifiers::NodeData
                               || parentNode.getType() == ValueTreeIdentifiers::AlternativeNodeData
                               || parentNode.getType() == ValueTreeIdentifiers::RootNodeData;

    if (!parentNode.isValid() || !parentIsNoteNode) {
        setDefaultNodeNote(state, newNodeId, undoManager);
        return;
    }

    for (const juce::Identifier& property : inheritedProperties) {
        if (parentNode.hasProperty(property)) {
            newNode.setProperty(property, parentNode.getProperty(property), undoManager);
        }
    }

    if (!parentMidi.isValid() || parentMidi.getNumChildren() == 0) {
        setDefaultNodeNote(state, newNodeId, undoManager);
        return;
    }

    for (int noteIndex = 0; noteIndex < parentMidi.getNumChildren(); ++noteIndex) {
        newMidi.addChild(parentMidi.getChild(noteIndex).createCopy(), -1, undoManager);
    }
}
