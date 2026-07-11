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
#include "../UI/Node/DanglingArrow.h"
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

    if (nodeValueTree.getType() == ValueTreeIdentifiers::TraversalData) {
        rebuildGraphsForTraversal(nodeValueTree.getProperty(ValueTreeIdentifiers::TraversalId));
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

void RTGraphBuilder::rebuildGraphsForTraversal(int traversalId)
{
    juce::ValueTree nodeMap = ValueTreeState::nodeMap;

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        juce::ValueTree rootNode = nodeMap.getChild(i);

        if (rootNode.getType() != ValueTreeIdentifiers::RootNodeData) {
            continue;
        }

        juce::ValueTree traversalChildrenIds = rootNode.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        if (traversalChildrenIds.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {
            makeRTGraph(rootNode);
        }
    }
}

void RTGraphBuilder::createRTNodes(juce::ValueTree rootNodeValueTree, std::shared_ptr<RTGraph> rtGraph, std::unordered_map<int, juce::ValueTree>& tempNodeMap) {
    std::vector<juce::ValueTree> stack = {rootNodeValueTree};

    while(!stack.empty()) {

        juce::ValueTree currentValueTree = stack.back();
        juce::ValueTree nodeParentValueTree = ValueTreeState::getNodeParent(currentValueTree.getProperty(ValueTreeIdentifiers::Id));

        juce::ValueTree nodeValueTreeChildren = currentValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);
        juce::ValueTree nodeValueTreeTraversals = currentValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        juce::ValueTree nodeMidiNotes = currentValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

        juce::Identifier nodeType = currentValueTree.getType();

        stack.pop_back();

        int nodeId      = currentValueTree.getProperty(ValueTreeIdentifiers::Id);
        int graphId     = currentValueTree.getProperty(ValueTreeIdentifiers::RootNodeId);

        int countLimit       = currentValueTree.getProperty(ValueTreeIdentifiers::CountLimit);
        int triggerLimit     = currentValueTree.getProperty(ValueTreeIdentifiers::TriggerLimit, ValueTreeState::defaultTriggerLimit);
        int switchCountLimit = currentValueTree.getProperty(ValueTreeIdentifiers::SwitchCountLimit);
        int subLoopLimit     = currentValueTree.getProperty(ValueTreeIdentifiers::SubLoopCountLimit);

        int repeatValue = currentValueTree.getProperty(ValueTreeIdentifiers::RepeatValue, ValueTreeState::defaultRepeatValue);

        bool isAlternativeNode = (nodeType == ValueTreeIdentifiers::AlternativeNodeData);

        if(tempNodeMap.count(nodeId) == false) {

            tempNodeMap[nodeId] = currentValueTree;

            RTNode rtNode;
            RTNode* parentNode = nullptr;

            rtNode.graphID = graphId;
            rtNode.nodeID  = nodeId;

            rtNode.countLimit        =  countLimit;
            rtNode.triggerLimit      = triggerLimit;
            rtNode.subLoopCountLimit = subLoopLimit;
            rtNode.switchCountLimit  = switchCountLimit;
            rtNode.repeatValue       = repeatValue;

            rtNode.isAlternativeNode = isAlternativeNode;

            for (int i = 0; i < nodeValueTreeTraversals.getNumChildren(); i++) {
                juce::ValueTree traversalIdTree = nodeValueTreeTraversals.getChild(i);
                int traversalId = traversalIdTree.getProperty(ValueTreeIdentifiers::TraversalId);
                juce::ValueTree traversalData = ValueTreeState::traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);
                RTtraversal rtTraversal;

                rtTraversal.traversalId = traversalId;

                if (traversalData.isValid()) {
                    rtTraversal.tempoMultiplier = traversalData.getProperty(ValueTreeIdentifiers::TempoMultiplier);
                }

                rtNode.traversals.push_back(rtTraversal);
            }

            if (nodeParentValueTree.isValid()) {
                int candidateParentId = nodeParentValueTree.getProperty(ValueTreeIdentifiers::Id);
                auto parentIt = rtGraph->nodeMap.find(candidateParentId);

                if (parentIt != rtGraph->nodeMap.end()) {
                    rtNode.parentId = candidateParentId;
                    parentNode = &parentIt->second;

                    if (isAlternativeNode) {
                        if (parentNode->nodeType != RTNode::NodeType::Alternative) {
                            rtNode.isAlternativeRoot = true;
                            parentNode->alternativeRootId = nodeId;
                            rtNode.alternativeRootId = nodeId;
                        }
                        else if (parentNode->nodeType == RTNode::NodeType::Alternative) {
                            rtNode.isAlternativeNode = true;
                            rtNode.alternativeRootId = parentNode->alternativeRootId;
                        }
                    }
                    if (parentNode->isConnectedToModulator == true) {
                        rtNode.isConnectedToModulator = true;
                    }
                }
            }

            auto nodeIt = canvas.nodeMap.find(nodeId);

            if (nodeIt != canvas.nodeMap.end()) {
                Node* nodeFromTree = nodeIt->second;
                for (auto& [childId, nodeArrow] : nodeFromTree->nodeArrows) {
                    rtNode.durationMap[childId] = nodeArrow->duration;
                }
                for (DanglingArrow* danglingArrow : canvas.danglingArrows) {
                    if (danglingArrow->startNode == nodeFromTree) {
                        rtNode.durationMap[nodeId] = danglingArrow->getDuration();
                    }
                }
            }


            if (nodeType == ValueTreeIdentifiers::NodeData) {
                rtNode.nodeType = RTNode::NodeType::Node;
            }
            if (nodeType == ValueTreeIdentifiers::AlternativeNodeData) {
                rtNode.nodeType = RTNode::NodeType::Alternative;
            }
            if (nodeType == ValueTreeIdentifiers::RootNodeData) {
                rtNode.nodeType = RTNode::NodeType::RootNode;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorRootData) {
                rtNode.nodeType = RTNode::NodeType::ModulatorRoot;
            }
            if (nodeType == ValueTreeIdentifiers::ModulatorData) {
                rtNode.nodeType = RTNode::NodeType::Modulator;
            }
            if (nodeType == ValueTreeIdentifiers::TraversalFlagData) {
                rtNode.nodeType = RTNode::NodeType::TraversalFlagData;
            }


            for (int i = 0; i < nodeMidiNotes.getNumChildren(); i++) {
                juce::ValueTree note = nodeMidiNotes.getChild(i);
                RTNote rtNote;

                int pitch       = note.getProperty(ValueTreeIdentifiers::MidiPitch);
                int velocity    = note.getProperty(ValueTreeIdentifiers::MidiVelocity);
                int duration    = note.getProperty(ValueTreeIdentifiers::MidiDuration);
                int midiChannel = note.getProperty(ValueTreeIdentifiers::MidiChannel, ValueTreeState::defaultMidiChannel);

                rtNote.pitch       = pitch;
                rtNote.velocity    = velocity;
                rtNote.duration    = duration;
                rtNote.midiChannel = juce::jlimit(1, 16, midiChannel);
                rtNode.notes.push_back(std::move(rtNote));
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

            if (nodeValueTreeChildren.getNumChildren() == 0) {
                rtNode.isLeafNode = true;
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
    if (!snap || !snap->globalNodes || !snap->nodeStates) {
        return;
    }

    auto nodeIt = canvas.nodeMap.find(nodeId);
    if (nodeIt == canvas.nodeMap.end()) {
        return;
    }

    Node* node = nodeIt->second;

    juce::ValueTree nodeVT = ValueTreeState::getNode(nodeId);
    if (!nodeVT.isValid()) {
        return;
    }

    auto newSnap = std::make_shared<SequenceTreeAudioProcessor::AudioSnapshot>();
    newSnap->globalNodes = std::make_shared<NodeMap>(*snap->globalNodes);
    newSnap->nodeStates  = std::make_shared<NodeStateMap>(*snap->nodeStates);
    newSnap->rtGraphs    = snap->rtGraphs;

    auto globalNodeIt = newSnap->globalNodes->find(nodeId);
    if (globalNodeIt != newSnap->globalNodes->end()) {
        globalNodeIt->second.durationMap.clear();
        for (auto& [childId, arrow] : node->nodeArrows)
            globalNodeIt->second.durationMap[childId] = arrow->duration;
        for (DanglingArrow* danglingArrow : canvas.danglingArrows) {
            if (danglingArrow->startNode == node) {
                globalNodeIt->second.durationMap[nodeId] = danglingArrow->getDuration();
            }
        }
    }

    juce::ValueTree parentVT = ValueTreeState::getNodeParent(nodeId);
    if (parentVT.isValid()) {
        int parentId = parentVT.getProperty(ValueTreeIdentifiers::Id);
        auto parentIt = canvas.nodeMap.find(parentId);
        if (parentIt != canvas.nodeMap.end()) {
            Node* parentNode = parentIt->second;
            auto globalParentIt = newSnap->globalNodes->find(parentId);
            if (globalParentIt != newSnap->globalNodes->end()) {
                globalParentIt->second.durationMap.clear();
                for (auto& [childId, arrow] : parentNode->nodeArrows) {
                    globalParentIt->second.durationMap[childId] = arrow->duration;
                }
                for (DanglingArrow* danglingArrow : canvas.danglingArrows) {
                    if (danglingArrow->startNode == parentNode) {
                        globalParentIt->second.durationMap[parentId] = danglingArrow->getDuration();
                    }
                }
            }
        }
    }

    std::atomic_store(&processor->audioSnapshot, newSnap);
}

void RTGraphBuilder::destroyRTGraph(Node* root) { }
