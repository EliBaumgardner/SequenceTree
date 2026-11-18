#include "NodeFactory.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <utility>

NodeFactory::NodeFactory(GraphState& state, juce::UndoManager* undoManager)
    : state      (state),
      undoManager(undoManager)
{
}

void NodeFactory::createRootNode(const NodeInfo::Position& nodePosition)
{
    const juce::ValueTree rootNodeValueTree = state.addRootNode(undoManager);
    const int             rootId            = rootNodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.setNodePosition(rootNodeValueTree, nodePosition, undoManager);

    setDefaultNodeNote(rootId);

    setDefaultTraversal(rootId);
}

void NodeFactory::createDanglingArrow(juce::ValueTree nodeTree, const juce::Point<int>& tipOffset, ArrowInfo::Settings arrowInfo)
{
    if (!nodeTree.isValid()) {
        return;
    }

    juce::ValueTree arrowList = nodeTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);
    juce::ValueTree arrowTree(ArsNovaUI::CanvasIdentifiers::DanglingArrow);

    ArrowProperties arrowProperties(arrowTree);

    inheritArrowBindings(arrowInfo, nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), {});

    if (!arrowList.isValid()) {
        arrowList = juce::ValueTree(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

        nodeTree.addChild(arrowList, -1, undoManager);
    }

    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);

    arrowProperties.countLimit.setValue(GraphState::defaultNodeCountLimit, undoManager);

    ArrowBindingOps::setArrowInfo(arrowTree, arrowInfo, undoManager);

    arrowList.addChild(arrowTree, -1, undoManager);

    state.arrows.syncPitchBindings(nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), undoManager);
}

void NodeFactory::setDanglingArrowTip(juce::ValueTree arrowTree, const juce::Point<int>& tipOffset)
{
    if (!arrowTree.isValid()) {
        return;
    }

    ArrowProperties arrowProperties(arrowTree);

    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY, tipOffset.y, undoManager);

    arrowProperties.durationOverride.setValue(ArrowInfo::noDurationOverride, undoManager);

    state.arrows.syncPitchBindings(arrowTree.getParent().getParent().getProperty(ArsNovaUI::CanvasIdentifiers::Id), undoManager);
}

void NodeFactory::destroyDanglingArrow(juce::ValueTree arrowTree)
{
    juce::ValueTree arrowList = arrowTree.getParent();

    if (arrowList.isValid()) {
        arrowList.removeChild(arrowTree, undoManager);
    }
}

void NodeFactory::applyArrowInfo(int parentNodeId, int childNodeId, ArrowInfo::Settings arrowInfo, ArrowInfo::Type rootConnectionType)
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

    inheritArrowBindings(arrowInfo, parentNodeId, connection);

    ArrowBindingOps::setArrowInfo(connection, arrowInfo, undoManager);

    state.arrows.syncPitchBindings(childNodeId, undoManager);
}

juce::ValueTree NodeFactory::create(NodeCreationMode mode, int parentNodeId, bool makeAlternative, const NodeInfo::Position& position)
{
    const NodeInfo::KindTraits& parentTraits = NodeInfo::kindTraits[static_cast<int>(GraphState::kindOf(state.getNode(parentNodeId)))];

    switch (mode)
    {
        case NodeCreationMode::Node:
        {
            if (makeAlternative) {
                return createNode(parentNodeId, ValueTreeIdentifiers::AlternativeNodeData, position);
            }

            return createNode(parentNodeId, ValueTreeIdentifiers::NodeData, position);
        }

        case NodeCreationMode::Modulator:
        {
            if (parentTraits.isModulator) {
                if (makeAlternative) {
                    return createModulatorNode(NodeInfo::Kind::AlternativeModulator, parentNodeId, position);
                }

                return createModulatorNode(NodeInfo::Kind::Modulator, parentNodeId, position);
            }

            return createModulatorNode(NodeInfo::Kind::ModulatorRoot, parentNodeId, position);
        }

        case NodeCreationMode::TraversalFlag:
        {
            return createTraversalFlagNode(parentNodeId, position);
        }
    }

    return {};
}

juce::ValueTree NodeFactory::createNode(int parentNodeId, const juce::Identifier& nodeType, const NodeInfo::Position& nodePosition)
{
    const juce::ValueTree childNodeValueTree = state.addChildNode(parentNodeId, nodeType, undoManager);
    const int             nodeId             = childNodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

    state.setNodePosition(childNodeValueTree, nodePosition, undoManager);

    inheritFromParent(parentNodeId, nodeId, childNodeValueTree);

    return childNodeValueTree;
}

juce::ValueTree NodeFactory::createTraversalFlagNode(int parentNodeId, const NodeInfo::Position& nodePosition)
{
    const juce::ValueTree childNodeValueTree = state.addTraversalFlagNode(parentNodeId, undoManager);

    state.setNodePosition(childNodeValueTree, nodePosition, undoManager);

    return childNodeValueTree;
}

juce::ValueTree NodeFactory::createEncapsulator(std::span<const int> memberNodeIds)
{
    const int       encapsulatorLabel     = state.encapsulation.unusedLabel();
    juce::ValueTree encapsulatorValueTree = state.encapsulation.create(memberNodeIds, undoManager);

    NodeProperties encapsulatorProperties(encapsulatorValueTree);

    encapsulatorProperties.encapsulatorLabel.setValue(encapsulatorLabel, undoManager);

    const NodeInfo::Position memberPosition = state.getNodePosition(memberNodeIds.front());

    state.setNodePosition(encapsulatorValueTree, memberPosition, undoManager);

    return encapsulatorValueTree;
}

juce::ValueTree NodeFactory::createModulatorNode(NodeInfo::Kind kind, int parentNodeId, const NodeInfo::Position& nodePosition)
{
    const juce::ValueTree modulatorValueTree = state.addModulator(parentNodeId, kind, undoManager);

    state.setNodePosition(modulatorValueTree, nodePosition, undoManager);

    return modulatorValueTree;
}

void NodeFactory::setDefaultNodeNote(int nodeId)
{
    NodeInfo::Note note;

    note.pitch       = NodeInfo::defaultMidiPitch;
    note.velocity    = NodeInfo::defaultMidiVelocity;
    note.duration    = NodeInfo::defaultNoteDurationMs;
    note.midiChannel = GraphState::defaultMidiChannel;

    state.addMidiNote(nodeId, note, undoManager);
}

void NodeFactory::setDefaultTraversal(int nodeId)
{
    const juce::ValueTree rootNodeValueTree    = state.getNode(nodeId);
    const int             traversalInstance    = state.traversals.unusedInstance(TraversalState::defaultTraversalId);
    juce::ValueTree       traversalChildrenIds = rootNodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);
    juce::ValueTree       traversalId          { ValueTreeIdentifiers::TraversalId };

    TraversalReferenceProperties reference(traversalId);

    state.traversals.addTraversalData(TraversalState::defaultTraversalId, undoManager);

    reference.traversalId      .setValue(TraversalState::defaultTraversalId, undoManager);
    reference.traversalInstance.setValue(traversalInstance,                  undoManager);

    traversalChildrenIds.addChild(traversalId, -1, undoManager);
}

void NodeFactory::inheritFromParent(int parentNodeId, int newNodeId, juce::ValueTree newNode)
{
    const juce::ValueTree  parentNode         = state.getNode(parentNodeId);
    const juce::ValueTree  parentMidi         = parentNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
    juce::ValueTree        newMidi            = newNode.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    const NodeInfo::KindTraits& parentTraits = NodeInfo::kindTraits[static_cast<int>(GraphState::kindOf(parentNode))];

    if (!parentNode.isValid() || !parentTraits.playsNotes) {
        setDefaultNodeNote(newNodeId);

        return;
    }

    for (const NodePropertyTable::Row& row : NodePropertyTable::rows) {
        if (row.isInheritedByChild && parentNode.hasProperty(row.identifier)) {
            newNode.setProperty(row.identifier, parentNode.getProperty(row.identifier), undoManager);
        }
    }

    if (!parentMidi.isValid() || parentMidi.getNumChildren() == 0) {
        setDefaultNodeNote(newNodeId);

        return;
    }

    for (int noteIndex = 0; noteIndex < parentMidi.getNumChildren(); ++noteIndex) {
        newMidi.addChild(parentMidi.getChild(noteIndex).createCopy(), -1, undoManager);
    }
}

void NodeFactory::inheritArrowBindings(ArrowInfo::Settings& arrowInfo, int nodeId, const juce::ValueTree& newArrow) const
{
    const juce::ValueTree node = state.getNode(nodeId);

    if (! node.isValid()) {
        return;
    }

    auto targetsAlternative = [this](const juce::ValueTree& arrowTree) {
        if (! arrowTree.hasProperty(ArsNovaUI::CanvasIdentifiers::Id)) {
            return false;
        }

        const int targetId = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        const NodeInfo::KindTraits& targetTraits = NodeInfo::kindTraits[static_cast<int>(GraphState::kindOf(state.getNode(targetId)))];

        return targetTraits.isAlternative;
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
