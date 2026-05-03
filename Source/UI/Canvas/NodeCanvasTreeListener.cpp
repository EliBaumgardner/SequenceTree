#include "NodeCanvasTreeListener.h"
#include "NodeCanvas.h"
#include "../../Graph/ValueTreeIdentifiers.h"

NodeCanvasTreeListener::NodeCanvasTreeListener(NodeCanvas& canvasIn) : canvas(canvasIn) {}

void NodeCanvasTreeListener::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap)
    {
        jassert(child.getType() == ValueTreeIdentifiers::NodeData
            || child.getType() == ValueTreeIdentifiers::ConnectorData
            || child.getType() == ValueTreeIdentifiers::RootNodeData
            || child.getType() == ValueTreeIdentifiers::ModulatorRootData
            || child.getType() == ValueTreeIdentifiers::ModulatorData);

        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::NodeAdded;
        update.nodeId     = child.getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId);
        canvas.enqueueAsyncUpdate(update);
    }
}

void NodeCanvasTreeListener::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int /*childIndex*/)
{
    if (parent.getType() == ValueTreeIdentifiers::NodeMap)
    {
        NodeCanvas::AsyncUpdate update;
        update.type       = NodeCanvas::AsyncUpdateType::NodeRemoved;
        update.nodeId     = child.getProperty(ValueTreeIdentifiers::Id);
        update.rootNodeId = child.getProperty(ValueTreeIdentifiers::RootNodeId);
        canvas.enqueueAsyncUpdate(update);
    }
}

void NodeCanvasTreeListener::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier)
{
    const juce::Identifier nodeType = tree.getType();

    if (propertyIdentifier == ValueTreeIdentifiers::XPosition
        || propertyIdentifier == ValueTreeIdentifiers::YPosition
        || propertyIdentifier == ValueTreeIdentifiers::Radius)
    {
        jassert(nodeType == ValueTreeIdentifiers::NodeData
            || nodeType == ValueTreeIdentifiers::ConnectorData
            || nodeType == ValueTreeIdentifiers::RootNodeData
            || nodeType == ValueTreeIdentifiers::ModulatorRootData
            || nodeType == ValueTreeIdentifiers::ModulatorData);

        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::NodeMoved;
        update.nodeId = tree.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiDuration)
    {
        juce::ValueTree noteNode = tree.getParent().getParent();
        NodeCanvas::AsyncUpdate update;
        update.type   = NodeCanvas::AsyncUpdateType::DurationOnly;
        update.nodeId = noteNode.getProperty(ValueTreeIdentifiers::Id);
        canvas.enqueueAsyncUpdate(update);
    }
}
