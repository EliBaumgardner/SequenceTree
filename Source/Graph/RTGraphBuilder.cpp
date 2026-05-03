/*
  ==============================================================================

    RTGraphBuilder.cpp
    Created: 30 Apr 2026
    Author:  Eli Baumgardner

  ==============================================================================
*/

#include "RTGraphBuilder.h"

#include "../Plugin/PluginProcessor.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/NodeArrow.h"
#include "../Util/ApplicationContext.h"
#include "ValueTreeState.h"
#include "ValueTreeIdentifiers.h"


RTGraphBuilder::RTGraphBuilder(ApplicationContext& context, NodeCanvas& canvasRef)
    : applicationContext(context), canvas(canvasRef)
{
}

void RTGraphBuilder::makeRTGraph(const juce::ValueTree& nodeValueTree)
{
    if (!nodeValueTree.isValid()) {
        return;
    }

    int rootNodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
    if (rootNodeId == 0) {
        return;
    }

    juce::ValueTree rootNodeValueTree = ValueTreeState::getNode(rootNodeId);
    if (!rootNodeValueTree.isValid()) {
        rtGraphs.erase(rootNodeId);
        auto emptyGraph = std::make_shared<RTGraph>();
        emptyGraph->graphID = rootNodeId;
        applicationContext.processor->setNewGraph(emptyGraph);
        return;
    }

    std::shared_ptr<RTGraph> rtGraph = std::make_shared<RTGraph>();
    std::unordered_map<int,juce::ValueTree> tempNodeMap;

    rtGraph->graphID   = rootNodeId;
    rtGraph->loopLimit = rootNodeValueTree.getProperty(ValueTreeIdentifiers::LoopLimit, 0);

    createRTNodes(rootNodeValueTree, rtGraph, tempNodeMap);
    createRTNodeConnections(rtGraph, tempNodeMap);

    rtGraph->traversalRequested = canvas.start;

    rtGraphs[rtGraph->graphID] = rtGraph;
    applicationContext.processor->setNewGraph(rtGraph);
}

void RTGraphBuilder::createRTNodes(juce::ValueTree rootNodeValueTree, std::shared_ptr<RTGraph> rtGraph, std::unordered_map<int, juce::ValueTree>& tempNodeMap) {
    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()){

        juce::ValueTree currentValueTree = stack.back();
        juce::ValueTree nodeValueTreeChildren = currentValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeMidiNotes = currentValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
        juce::ValueTree nodeParentValueTree = ValueTreeState::getNodeParent(currentValueTree.getProperty(ValueTreeIdentifiers::Id));

        juce::Identifier nodeType = currentValueTree.getType();
        juce::Identifier nodeParentType = nodeParentValueTree.getType();

        stack.pop_back();

        int nodeId = currentValueTree.getProperty(ValueTreeIdentifiers::Id);
        int graphId = currentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);
        int countLimit = currentValueTree.getProperty(ValueTreeIdentifiers::CountLimit);

        if(tempNodeMap.count(nodeId) == false){

            tempNodeMap[nodeId] = currentValueTree;

            RTNode rtNode;
            RTNode* parentNode = nullptr;

            rtNode.graphID = graphId;
            rtNode.nodeID = nodeId;

            if (nodeParentValueTree.isValid()) {
                rtNode.parentId = nodeParentValueTree.getProperty(ValueTreeIdentifiers::Id);
                auto parentIt = rtGraph->nodeMap.find(rtNode.parentId);
                if (parentIt != rtGraph->nodeMap.end()) {
                    parentNode = &parentIt->second;
                }
            }

            rtNode.countLimit = countLimit;

            auto nodeIt = canvas.nodeMap.find(nodeId);

            if (nodeIt != canvas.nodeMap.end()) {
                Node* nodeFromTree = nodeIt->second;
                for (auto& [childId, nodeArrow] : nodeFromTree->nodeArrows) {
                    rtNode.durationMap[childId] = nodeArrow->duration;
                }
            }

            if (nodeType == ValueTreeIdentifiers::NodeData) {
                rtNode.nodeType = RTNode::NodeType::Node;
            }
            if (nodeType == ValueTreeIdentifiers::RootNodeData) {
                rtNode.nodeType = RTNode::NodeType::RootNode;
            }
            if (nodeType == ValueTreeIdentifiers::ConnectorData) {
                rtNode.nodeType = RTNode::NodeType::Connector;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorRootData) {
                rtNode.nodeType = RTNode::NodeType::ModulatorRoot;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorData) {
                rtNode.nodeType = RTNode::NodeType::Modulator;
            }
            if (nodeParentType == ValueTreeIdentifiers::ConnectorData) {
                rtNode.graphID = rtNode.nodeID;
            }

            for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
                juce::ValueTree note = nodeMidiNotes.getChild(i);
                RTNote rtNote;

                int pitch   = note.getProperty(ValueTreeIdentifiers::MidiPitch);
                int velocity= note.getProperty(ValueTreeIdentifiers::MidiVelocity);
                int duration= note.getProperty(ValueTreeIdentifiers::MidiDuration);

                rtNote.pitch    = pitch;
                rtNote.velocity = velocity;
                rtNote.duration = duration;
                rtNode.notes.push_back(std::move(rtNote));
            }

            if (parentNode != nullptr) {
                if (parentNode->isConnectedToModulator == true) {
                    rtNode.isConnectedToModulator = true;
                }
            }

            for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
                juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
                int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);
                juce::ValueTree childDataTree = ValueTreeState::getNode(childId);

               if (childDataTree.getType() == ValueTreeIdentifiers::ModulatorRootData) {
                   rtNode.isConnectedToModulator = true;
               }

                jassert(childDataTree.isValid());
                stack.push_back(childDataTree);
            }

            rtGraph->nodeMap[nodeId] = std::move(rtNode);
        }
    }
}

void RTGraphBuilder::createRTNodeConnections(std::shared_ptr<RTGraph> rtGraph, std::unordered_map<int, juce::ValueTree>& tempNodeMap)
{
    for (auto& [id, nodeValueTree] : tempNodeMap) {
        juce::ValueTree nodeChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        for (int i = 0; i < nodeChildrenIds.getNumChildren(); i++) {
            juce::ValueTree childIdTree = nodeChildrenIds.getChild(i);
            int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

            juce::ValueTree childDataTree = ValueTreeState::getNode(childId);

            if (!childDataTree.isValid()) {
                continue;
            }

            rtGraph->nodeMap[id].children.push_back(childId);
        }
    }
}

void RTGraphBuilder::updateDurationMap(int nodeId)
{
    auto* processor = applicationContext.processor;
    auto snap = std::atomic_load(&processor->audioSnapshot);
    if (!snap || !snap->globalNodes) return;

    auto nodeIt = canvas.nodeMap.find(nodeId);
    if (nodeIt == canvas.nodeMap.end()) return;
    Node* node = nodeIt->second;

    juce::ValueTree nodeVT = ValueTreeState::getNode(nodeId);
    if (!nodeVT.isValid()) return;

    auto newSnap = std::make_shared<SequenceTreeAudioProcessor::AudioSnapshot>();
    newSnap->globalNodes = std::make_shared<NodeMap>(*snap->globalNodes);
    newSnap->rtGraphs    = snap->rtGraphs;

    auto globalNodeIt = newSnap->globalNodes->find(nodeId);
    if (globalNodeIt != newSnap->globalNodes->end())
    {
        globalNodeIt->second.durationMap.clear();
        for (auto& [childId, arrow] : node->nodeArrows)
            globalNodeIt->second.durationMap[childId] = arrow->duration;
    }

    juce::ValueTree parentVT = ValueTreeState::getNodeParent(nodeId);
    if (parentVT.isValid())
    {
        int parentId = parentVT.getProperty(ValueTreeIdentifiers::Id);
        auto parentIt = canvas.nodeMap.find(parentId);
        if (parentIt != canvas.nodeMap.end())
        {
            Node* parentNode = parentIt->second;
            auto globalParentIt = newSnap->globalNodes->find(parentId);
            if (globalParentIt != newSnap->globalNodes->end())
            {
                globalParentIt->second.durationMap.clear();
                for (auto& [childId, arrow] : parentNode->nodeArrows) {
                    globalParentIt->second.durationMap[childId] = arrow->duration;
                }
            }
        }
    }

    std::atomic_store(&processor->audioSnapshot, newSnap);
}

void RTGraphBuilder::destroyRTGraph(Node* root) { }
