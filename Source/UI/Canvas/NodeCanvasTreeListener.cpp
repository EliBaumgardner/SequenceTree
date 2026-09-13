#include "NodeCanvasTreeListener.h"
#include "NodeCanvas.h"
#include "../../Graph/ValueTreeIdentifiers.h"

NodeCanvasTreeListener::NodeCanvasTreeListener(NodeCanvas& canvasIn) : canvas(canvasIn) {}

void NodeCanvasTreeListener::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        jassert(child.getType() == ValueTreeIdentifiers::NodeData
            || child.getType() == ValueTreeIdentifiers::AlternativeNodeData
            || child.getType() == ValueTreeIdentifiers::RootNodeData
            || child.getType() == ValueTreeIdentifiers::ModulatorRootData
            || child.getType() == ValueTreeIdentifiers::ModulatorData
            || child.getType() == ValueTreeIdentifiers::AlternativeModulatorData
            || child.getType() == ValueTreeIdentifiers::TraversalFlagData
            || child.getType() == ValueTreeIdentifiers::EncapsulatorData);

        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::NodeAdded;
        update.nodeId     = child.getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::ArrowAdded;
        update.nodeId     = parent.getParent().getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = child.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (parent.getType() == ValueTreeIdentifiers::TraversalChildrenIds
          || parent.getType() == ValueTreeIdentifiers::DisabledTraversalIds) {
        enqueueGraphRebuild(parent);
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrows) {
        enqueueDanglingArrowsChanged(parent);
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrow) {
        enqueueDanglingArrowsChanged(parent.getParent());
    }
}

void NodeCanvasTreeListener::enqueueDanglingArrowsChanged(const juce::ValueTree& nodeTree) const
{
    if (! nodeTree.isValid()) {
        return;
    }

    NodeCanvas::AsyncUpdate update;
    update.type   = NodeCanvas::AsyncUpdateType::DanglingArrowsChanged;
    update.nodeId = nodeTree.getProperty(ValueTreeIdentifiers::Id);
    canvas.enqueueAsyncUpdate(update);
}

void NodeCanvasTreeListener::enqueueGraphRebuild(juce::ValueTree tree) const
{
    while (tree.isValid() && ! tree.hasProperty(ValueTreeIdentifiers::RootNodeId)) {
        tree = tree.getParent();
    }

    if (! tree.isValid()) {
        return;
    }

    NodeCanvas::AsyncUpdate update;
    update.type   = NodeCanvas::AsyncUpdateType::GraphRebuild;
    update.nodeId = tree.getProperty(ValueTreeIdentifiers::Id);
    canvas.enqueueAsyncUpdate(update);
}

void NodeCanvasTreeListener::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::NodeRemoved;
        update.nodeId     = child.getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::ArrowRemoved;
        update.nodeId     = parent.getParent().getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = child.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (parent.getType() == ValueTreeIdentifiers::TraversalChildrenIds
          || parent.getType() == ValueTreeIdentifiers::DisabledTraversalIds) {
        enqueueGraphRebuild(parent);
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrows) {
        enqueueDanglingArrowsChanged(parent);
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrow) {
        enqueueDanglingArrowsChanged(parent.getParent());
    }
}

void NodeCanvasTreeListener::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier)
{
    const juce::Identifier nodeType = tree.getType();

    if (propertyIdentifier == ValueTreeIdentifiers::XPosition
        || propertyIdentifier == ValueTreeIdentifiers::YPosition
        || propertyIdentifier == ValueTreeIdentifiers::Radius) {

        jassert(nodeType == ValueTreeIdentifiers::NodeData
            || nodeType == ValueTreeIdentifiers::AlternativeNodeData
            || nodeType == ValueTreeIdentifiers::RootNodeData
            || nodeType == ValueTreeIdentifiers::ModulatorRootData
            || nodeType == ValueTreeIdentifiers::ModulatorData
            || nodeType == ValueTreeIdentifiers::AlternativeModulatorData
            || nodeType == ValueTreeIdentifiers::TraversalFlagData
            || nodeType == ValueTreeIdentifiers::EncapsulatorData);

        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::NodeMoved;
        update.nodeId = tree.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiDuration) {
        juce::ValueTree noteNode = tree.getParent().getParent();
        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::DurationOnly;
        update.nodeId = noteNode.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);

        enqueueGraphRebuild(noteNode);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiPitch
        || propertyIdentifier == ValueTreeIdentifiers::MidiVelocity) {

        juce::ValueTree noteNode = tree.getParent().getParent();
        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::ValueChanged;
        update.nodeId = noteNode.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);

        enqueueGraphRebuild(noteNode);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiChannel) {
        enqueueGraphRebuild(tree);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::CountLimit
        || propertyIdentifier == ValueTreeIdentifiers::TriggerLimit
        || propertyIdentifier == ValueTreeIdentifiers::LoopLimit
        || propertyIdentifier == ValueTreeIdentifiers::SwitchCountLimit
        || propertyIdentifier == ValueTreeIdentifiers::SubLoopCountLimit
        || propertyIdentifier == ValueTreeIdentifiers::RepeatValue
        || propertyIdentifier == ValueTreeIdentifiers::Probability
        || propertyIdentifier == ValueTreeIdentifiers::ModAmount
        || propertyIdentifier == ValueTreeIdentifiers::TraversalFlagValue
        || propertyIdentifier == ValueTreeIdentifiers::EncapsulatorId) {

        enqueueGraphRebuild(tree);
    }
    else if (nodeType == ValueTreeIdentifiers::TraversalData
        && (propertyIdentifier == ValueTreeIdentifiers::TempoMultiplier
         || propertyIdentifier == ValueTreeIdentifiers::TraversalChannel
         || propertyIdentifier == ValueTreeIdentifiers::TraversalTranspose
         || propertyIdentifier == ValueTreeIdentifiers::TraversalVelocity)) {

        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::TraversalDataChanged;
        update.nodeId = tree.getProperty(ValueTreeIdentifiers::TraversalId);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowTipX
        || propertyIdentifier == ValueTreeIdentifiers::ArrowTipY) {

        enqueueDanglingArrowsChanged(tree.getParent().getParent());
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowDuration) {
        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::ArrowDurationChanged;
        update.nodeId = tree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowType
        || propertyIdentifier == ValueTreeIdentifiers::ArrowSync
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXMultiplier
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYMultiplier) {
        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::ArrowInfoChanged;
        update.nodeId     = tree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = tree.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
}
