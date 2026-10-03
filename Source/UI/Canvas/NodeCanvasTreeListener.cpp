#include "NodeCanvasTreeListener.h"
#include "NodeCanvas.h"
#include "../../Graph/ValueTreeIdentifiers.h"

NodeCanvasTreeListener::NodeCanvasTreeListener(NodeCanvas& canvas) : canvas(canvas)
{
}

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

        canvas.enqueueAsyncUpdate({ .type       = NodeCanvas::AsyncUpdateType::NodeAdded,
                                    .nodeId     = child.getProperty(ValueTreeIdentifiers::Id),
                                    .rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId) });
    }
    else if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        canvas.enqueueAsyncUpdate({ .type       = NodeCanvas::AsyncUpdateType::ArrowAdded,
                                    .nodeId     = parent.getParent().getProperty(ValueTreeIdentifiers::Id),
                                    .rootNodeId = child.getProperty(ValueTreeIdentifiers::Id) });
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

    canvas.enqueueAsyncUpdate({ .type   = NodeCanvas::AsyncUpdateType::DanglingArrowsChanged,
                                .nodeId = nodeTree.getProperty(ValueTreeIdentifiers::Id) });
}

void NodeCanvasTreeListener::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap) {
        canvas.enqueueAsyncUpdate({ .type       = NodeCanvas::AsyncUpdateType::NodeRemoved,
                                    .nodeId     = child.getProperty(ValueTreeIdentifiers::Id),
                                    .rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId) });
    }
    else if (parent.getType() == ValueTreeIdentifiers::NodeChildrenIds) {
        canvas.enqueueAsyncUpdate({ .type       = NodeCanvas::AsyncUpdateType::ArrowRemoved,
                                    .nodeId     = parent.getParent().getProperty(ValueTreeIdentifiers::Id),
                                    .rootNodeId = child.getProperty(ValueTreeIdentifiers::Id) });
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

        canvas.enqueueAsyncUpdate({ .type   = NodeCanvas::AsyncUpdateType::NodeMoved,
                                    .nodeId = tree.getProperty(ValueTreeIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiPitch
        || propertyIdentifier == ValueTreeIdentifiers::MidiVelocity) {
        canvas.enqueueAsyncUpdate({ .type   = NodeCanvas::AsyncUpdateType::ValueChanged,
                                    .nodeId = tree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowTipX
        || propertyIdentifier == ValueTreeIdentifiers::ArrowTipY) {
        enqueueDanglingArrowsChanged(tree.getParent().getParent());
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::NodeColour) {
        canvas.enqueueAsyncUpdate({ .type   = NodeCanvas::AsyncUpdateType::NodeColourChanged,
                                    .nodeId = tree.getProperty(ValueTreeIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowDuration) {
        canvas.enqueueAsyncUpdate({ .type   = NodeCanvas::AsyncUpdateType::ArrowDurationChanged,
                                    .nodeId = tree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowType
        || propertyIdentifier == ValueTreeIdentifiers::ArrowSync
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXMultiplier
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYMultiplier) {
        canvas.enqueueAsyncUpdate({ .type       = NodeCanvas::AsyncUpdateType::ArrowInfoChanged,
                                    .nodeId     = tree.getParent().getParent().getProperty(ValueTreeIdentifiers::Id),
                                    .rootNodeId = tree.getProperty(ValueTreeIdentifiers::Id) });
    }
}
