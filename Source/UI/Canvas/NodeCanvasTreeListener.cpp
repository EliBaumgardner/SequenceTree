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
            || child.getType() == ValueTreeIdentifiers::TraversalFlagData);

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
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrows) {
        enqueueDanglingArrowsChanged(parent);
    }
    else if (child.getType() == ValueTreeIdentifiers::DanglingArrow) {
        enqueueDanglingArrowsChanged(parent.getParent());
    }
}

void NodeCanvasTreeListener::enqueueDanglingArrowsChanged(const juce::ValueTree& nodeTree)
{
    if (! nodeTree.isValid()) {
        return;
    }

    NodeCanvas::AsyncUpdate update;
    update.type   = NodeCanvas::AsyncUpdateType::DanglingArrowsChanged;
    update.nodeId = nodeTree.getProperty(ValueTreeIdentifiers::Id);
    canvas.enqueueAsyncUpdate(update);
}

void NodeCanvasTreeListener::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int /*childIndex*/)
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
            || nodeType == ValueTreeIdentifiers::TraversalFlagData);

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
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiPitch
        || propertyIdentifier == ValueTreeIdentifiers::MidiVelocity) {

        juce::ValueTree noteNode = tree.getParent().getParent();
        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::ValueChanged;
        update.nodeId = noteNode.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowTipX
        || propertyIdentifier == ValueTreeIdentifiers::ArrowTipY) {

        enqueueDanglingArrowsChanged(tree.getParent().getParent());
    }
}
