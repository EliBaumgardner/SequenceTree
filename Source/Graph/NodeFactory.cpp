#include "NodeFactory.h"
#include "ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <utility>

void NodeFactory::createRootNode(GraphState& state, const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager)
{
    const juce::ValueTree rootNodeValueTree = state.addRootNode(undoManager);
    const int             rootId            = rootNodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.setNodePosition(rootNodeValueTree, nodePosition, undoManager);

    setDefaultNodeNote(state, rootId, undoManager);
    setDefaultTraversal(state, rootId, undoManager);
}

void NodeFactory::createDanglingArrow(GraphState& state, juce::ValueTree nodeTree, const juce::Point<int>& tipOffset,
                                      ArrowInfo::Settings arrowInfo, juce::UndoManager* undoManager)
{
    if (!nodeTree.isValid()) {
        return;
    }

    juce::ValueTree arrowList = nodeTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);
    juce::ValueTree arrowTree(ArsNovaUI::CanvasIdentifiers::DanglingArrow);

    inheritArrowBindings(state, arrowInfo, nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), {});

    if (!arrowList.isValid()) {
        arrowList = juce::ValueTree(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        nodeTree.addChild(arrowList, -1, undoManager);
    }

    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::CountLimit, GraphState::defaultNodeCountLimit, undoManager);

    ArrowBindingOps::setArrowInfo(arrowTree, arrowInfo, undoManager);

    arrowList.addChild(arrowTree, -1, undoManager);

    state.arrows.syncPitchBindings(nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), undoManager);
}

void NodeFactory::setDanglingArrowTip(GraphState& state, juce::ValueTree arrowTree, const juce::Point<int>& tipOffset,
                                      juce::UndoManager* undoManager)
{
    if (!arrowTree.isValid()) {
        return;
    }

    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration, ArrowInfo::noDurationOverride, undoManager);

    state.arrows.syncPitchBindings(arrowTree.getParent().getParent().getProperty(ArsNovaUI::CanvasIdentifiers::Id), undoManager);
}

void NodeFactory::destroyDanglingArrow(juce::ValueTree arrowTree, juce::UndoManager* undoManager)
{
    juce::ValueTree arrowList = arrowTree.getParent();

    if (arrowList.isValid()) {
        arrowList.removeChild(arrowTree, undoManager);
    }
}

void NodeFactory::applyArrowInfo(GraphState& state, int parentNodeId, int childNodeId, ArrowInfo::Settings arrowInfo, ArrowInfo::Type rootConnectionType,
                                 juce::UndoManager* undoManager)
{
    const bool            connectsToOtherRoot  = state.connectsToOtherTreeRoot(parentNodeId, childNodeId);
    const bool            ownedByTraversalFlag = state.getNode(parentNodeId).getType() == ValueTreeIdentifiers::TraversalFlagData;
    const juce::ValueTree connection           = state.getConnection(parentNodeId, childNodeId);

    if (arrowInfo.type == ArrowInfo::Type::Traversal && ! connectsToOtherRoot) {
        arrowInfo.type = ArrowInfo::Type::Node;
    }

    if (connectsToOtherRoot && ! ownedByTraversalFlag && arrowInfo.type != ArrowInfo::Type::Traversal) {
        arrowInfo.type = rootConnectionType;
    }

    inheritArrowBindings(state, arrowInfo, parentNodeId, connection);

    ArrowBindingOps::setArrowInfo(connection, arrowInfo, undoManager);

    state.arrows.syncPitchBindings(childNodeId, undoManager);
}

juce::ValueTree NodeFactory::create(NodeCreationMode mode, GraphState& state, int parentNodeId, const juce::Identifier& parentType, bool makeAlternative,
                                    const NodeInfo::Position& position, juce::UndoManager* undoManager)
{
    switch (mode)
    {
        case NodeCreationMode::Node:
        {
            if (makeAlternative) {
                return createNode(state, parentNodeId, ValueTreeIdentifiers::AlternativeNodeData, position, undoManager);
            }

            return createNode(state, parentNodeId, ValueTreeIdentifiers::NodeData, position, undoManager);
        }

        case NodeCreationMode::Modulator:
        {
            if (parentType == ValueTreeIdentifiers::ModulatorRootData
                || parentType == ValueTreeIdentifiers::ModulatorData
                || parentType == ValueTreeIdentifiers::AlternativeModulatorData) {

                if (makeAlternative) {
                    return createAlternativeModulator(state, parentNodeId, position, undoManager);
                }

                return createModulator(state, parentNodeId, position, undoManager);
            }

            return createModulatorRoot(state, parentNodeId, position, undoManager);
        }

        case NodeCreationMode::TraversalFlag:
        {
            return createTraversalFlagNode(state, parentNodeId, position, undoManager);
        }
    }

    return {};
}

juce::ValueTree NodeFactory::createNode(GraphState& state, int parentNodeId, const juce::Identifier& nodeType,
                                        const NodeInfo::Position& nodePosition, juce::UndoManager* undoManager)
{
    const juce::ValueTree childNodeValueTree = state.addChildNode(parentNodeId, nodeType, undoManager);
    const int             nodeId             = childNodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.setNodePosition(childNodeValueTree, nodePosition, undoManager);

    inheritFromParent(state, parentNodeId, nodeId, childNodeValueTree, undoManager);

    return childNodeValueTree;
}

juce::ValueTree NodeFactory::createTraversalFlagNode(GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition,
                                                     juce::UndoManager* undoManager)
{
    const juce::ValueTree childNodeValueTree = state.addTraversalFlagNode(parentNodeId, undoManager);

    state.setNodePosition(childNodeValueTree, nodePosition, undoManager);

    return childNodeValueTree;
}

juce::ValueTree NodeFactory::createEncapsulator(GraphState& state, std::span<const int> memberNodeIds, juce::UndoManager* undoManager)
{
    const int       encapsulatorLabel     = state.encapsulation.unusedLabel();
    juce::ValueTree encapsulatorValueTree = state.encapsulation.create(memberNodeIds, undoManager);

    encapsulatorValueTree.setProperty(ValueTreeIdentifiers::EncapsulatorLabel, encapsulatorLabel, undoManager);

    state.setNodePosition(encapsulatorValueTree, state.getNodePosition(memberNodeIds.front()), undoManager);

    return encapsulatorValueTree;
}

juce::ValueTree NodeFactory::createModulatorRoot(GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition,
                                                 juce::UndoManager* undoManager)
{
    const juce::ValueTree modulatorRootValueTree = state.addModulatorRoot(parentNodeId, undoManager);

    state.setNodePosition(modulatorRootValueTree, nodePosition, undoManager);

    return modulatorRootValueTree;
}

juce::ValueTree NodeFactory::createModulator(GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition,
                                             juce::UndoManager* undoManager)
{
    const juce::ValueTree modulatorValueTree = state.addModulator(parentNodeId, undoManager);

    state.setNodePosition(modulatorValueTree, nodePosition, undoManager);

    return modulatorValueTree;
}

juce::ValueTree NodeFactory::createAlternativeModulator(GraphState& state, int parentNodeId, const NodeInfo::Position& nodePosition,
                                                        juce::UndoManager* undoManager)
{
    const juce::ValueTree alternativeModulatorValueTree = state.addAlternativeModulator(parentNodeId, undoManager);

    state.setNodePosition(alternativeModulatorValueTree, nodePosition, undoManager);

    return alternativeModulatorValueTree;
}

void NodeFactory::setDefaultNodeNote(GraphState& state, int nodeId, juce::UndoManager* undoManager)
{
    NodeInfo::Note note;

    note.pitch       = 60;
    note.velocity    = 60;
    note.duration    = 1000;
    note.midiChannel = GraphState::defaultMidiChannel;

    state.addMidiNote(nodeId, note, undoManager);
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

void NodeFactory::inheritArrowBindings(const GraphState& state, ArrowInfo::Settings& arrowInfo, int nodeId, const juce::ValueTree& newArrow)
{
    const juce::ValueTree node = state.getNode(nodeId);

    if (! node.isValid()) {
        return;
    }

    auto targetsAlternative = [&state](const juce::ValueTree& arrowTree) {
        if (! arrowTree.hasProperty(ArsNovaUI::CanvasIdentifiers::Id)) {
            return false;
        }

        const int targetId = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        const juce::Identifier targetType = state.getNode(targetId).getType();

        return targetType == ValueTreeIdentifiers::AlternativeNodeData
            || targetType == ValueTreeIdentifiers::AlternativeModulatorData;
    };

    juce::ValueTree establishedArrow;

    const juce::ValueTree childIds = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren() && ! establishedArrow.isValid(); ++i) {
        if (childIds.getChild(i) != newArrow && ! targetsAlternative(childIds.getChild(i))) {
            establishedArrow = childIds.getChild(i);
        }
    }

    const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren() && ! establishedArrow.isValid(); ++i) {
        if (danglingArrows.getChild(i) != newArrow) {
            establishedArrow = danglingArrows.getChild(i);
        }
    }

    if (establishedArrow.isValid()) {
        const ArrowInfo::Settings establishedInfo = ArrowBindingOps::getArrowInfo(establishedArrow);

        arrowInfo.xBinding    = establishedInfo.xBinding;
        arrowInfo.yBinding    = establishedInfo.yBinding;
        arrowInfo.xMultiplier = establishedInfo.xMultiplier;
        arrowInfo.yMultiplier = establishedInfo.yMultiplier;
    }

    if (targetsAlternative(newArrow)) {
        std::swap(arrowInfo.xBinding,    arrowInfo.yBinding);
        std::swap(arrowInfo.xMultiplier, arrowInfo.yMultiplier);

        if (arrowInfo.xBinding == ArrowInfo::Binding::PitchBind) {
            arrowInfo.xBinding = ArrowInfo::Binding::NoBind;
        }

        if (arrowInfo.yBinding == ArrowInfo::Binding::PitchBind) {
            arrowInfo.yBinding = ArrowInfo::Binding::NoBind;
        }
    }
}
