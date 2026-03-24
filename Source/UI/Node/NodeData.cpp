#include "NodeCanvas.h"
#include "Node.h"
#include "NodeData.h"


const juce::Identifier NodeData::nameID { "name" };
const juce::Identifier NodeData::channelID { "channel" };
const juce::Identifier NodeData::nodeType { "nodeType" };

const juce::Identifier NodeData::xID { "x" };
const juce::Identifier NodeData::yID { "y" };
const juce::Identifier NodeData::radiusID { "radius" };
const juce::Identifier NodeData::countID { "count" };
const juce::Identifier NodeData::countLimitID { "countLimit" };
const juce::Identifier NodeData::colourID { "colour" };
const juce::Identifier NodeData::nodeID { "nodeID" };

const juce::Identifier NodeData::pitchID { "pitch" };
const juce::Identifier NodeData::velocityID { "velocity" };
const juce::Identifier NodeData::durationID { "duration" };

const juce::Identifier NodeData::ccValueID { "ccValue" };
const juce::Identifier NodeData::ccNumberID { "ccNumber" };



NodeData::NodeData() : nodeData("NodeData"), nodeConnectors ("NodeConnectors"), nodeChildren("NodeChildren"), midiNotes("MidiNotes") {


    nodeData.addChild(nodeConnectors, -1, nullptr);
    nodeData.addChild(nodeChildren, -1, nullptr);
    nodeData.addChild(midiNotes, -1, nullptr);

    if (ComponentContext::canvas != nullptr) { ComponentContext::canvas->canvasTree.addChild(nodeData,-1,nullptr); }
    else { DBG("NODE CREATED WITHOUT CANVAS!"); }
}

NodeData::~NodeData() {
    for (auto& val : propertyValues)
        val = juce::Value();
    propertyValues.clear();
}

void NodeData::addChild(Node* child)
{
    DBG("child node added to nodeData");
    children.add(child);

    juce::ValueTree tree("NodeData");
    tree.setProperty("nodeID",child->nodeID,nullptr);
    tree.setProperty("nodeType",child->nodeData.nodeData.getProperty("nodeType"),nullptr );
    nodeData.addChild(tree, -1, nullptr);
}

void NodeData::removeChild(Node* child) {
    children.removeFirstMatchingValue(child);

    for (int i = nodeData.getNumChildren()-1; i >= 0; i--) {
        juce::ValueTree childNodeTree = nodeData.getChild(i);
        if (childNodeTree.getType() != juce::Identifier("NodeData")) { continue; }

        int childID = (int)childNodeTree.getProperty("nodeID");
        if (childID == child->nodeID) { nodeData.removeChild(i,nullptr); }
    }
}

void NodeData::addConnector(Node* connector) {
    connectors.add(connector);  connector->isConnector = true;
    juce::ValueTree tree("NodeData");
    tree.setProperty("nodeID",connector->nodeID,nullptr);
    tree.setProperty("nodeType",connector->nodeData.nodeData.getProperty("nodeType"), nullptr);
    nodeData.addChild(tree, -1, nullptr);
}

void NodeData::removeConnector(Node* connector) { connectors.removeFirstMatchingValue(connector); }

void NodeData::createTree(juce::String type)
{

    DBG("CREATING TREE");

    if(type == "MidiNoteData"){

        DBG("CREATING MIDI TREE");

        juce::ValueTree tree("MidiNoteData");
        tree.setProperty(channelID,0,nullptr);
        tree.setProperty(pitchID,63,nullptr);
        tree.setProperty(velocityID,63,nullptr);
        tree.setProperty(durationID,1000,nullptr);

        midiNotes.addChild(tree, -1, nullptr);

        tree.addListener(&listener);
        listener.onChanged = [this](){ ComponentContext::canvas->makeRTGraph(nullptr); };
    }

    ComponentContext::canvas->makeRTGraph(ComponentContext::canvas->root);
}

void NodeData::setProperty(juce::Identifier propertyID, juce::String propertyValue, juce::String type)
{
    DBG("setPropertyFunction Called");



    if (type == "NodeData")
    {
        DBG("setPropertyFunction Called with NodeData");
        nodeData.setProperty(propertyID,propertyValue, nullptr);
        DBG(nodeData.getProperty(propertyID).toString());
    }
    else if (type == "NodeConnectors") {
        DBG("setPropertyFunction Called with NodeConnectors");
        nodeConnectors.setProperty(propertyID, propertyValue, nullptr);
        DBG(nodeConnectors.getProperty(propertyID).toString());
    }
};

void NodeData::setNode(Node* node)
{
    this->node = node;
};
