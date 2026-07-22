//
// Created by Eli Baumgardner on 7/21/26.
//

#include "NodeManager.h"

#include "NodeCanvas.h"
#include "ArrowManager.h"
#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include "../Node/Modulator.h"
#include "../Node/TraversalFlagNode.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Util/ApplicationContext.h"

NodeManager::NodeManager(NodeCanvas& canvasRef, ApplicationContext& context)
    : canvas(canvasRef), applicationContext(context)
{
}

NodeManager::~NodeManager() = default;

Node* NodeManager::find(int nodeId) const
{
    auto nodePair = nodes.find(nodeId);
    return nodePair == nodes.end() ? nullptr : nodePair->second;
}

Node* NodeManager::instantiateFromTree(const juce::ValueTree& nodeValueTree)
{
    jassert(nodeValueTree.isValid());

    const juce::Identifier treeType = nodeValueTree.getType();
    const int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    std::unique_ptr<Node> node;
    if (treeType == ValueTreeIdentifiers::RootNodeData) {
        node = std::make_unique<RootNode>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::NodeData) {
        node = std::make_unique<Node>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::AlternativeNodeData) {
        node = std::make_unique<Node>(applicationContext);
        node.get()->isAlternativeNode = true;
    }
    else if (treeType == ValueTreeIdentifiers::TraversalFlagData) {
        node = std::make_unique<TraversalFlagNode>(applicationContext);
    }
    else if (treeType == ValueTreeIdentifiers::ModulatorData
          || treeType == ValueTreeIdentifiers::ModulatorRootData) {
        node = std::make_unique<Modulator>(applicationContext);
    }

    jassert(node);

    juce::ValueTree midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    node->setComponentID(std::to_string(nodeId));
    node->nodeValueTree = nodeValueTree;
    node->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node->setDisplayMode(NodeDisplayMode::Pitch);

    node->onSelected = [this](Node* n, bool sel) {
        applicationContext.notifyNodeSelected(n, sel);
    };

    canvas.addAndMakeVisible(node.get());
    node->setInterceptsMouseClicks(!canvas.paintMode, !canvas.paintMode);

    Node* raw = node.release();
    nodes[nodeId] = raw;

    setPosition(nodeId);

    return raw;
}

void NodeManager::add(int nodeId)
{
    juce::ValueTree nodeChildTree  = applicationContext.valueTreeState->getNode(nodeId);
    juce::ValueTree nodeParentTree = applicationContext.valueTreeState->getNodeParent(nodeId);

    jassert(nodeChildTree.isValid());

    Node* childNode = instantiateFromTree(nodeChildTree);

    if (nodeParentTree.isValid()) {
        int parentNodeId = nodeParentTree.getProperty(ValueTreeIdentifiers::Id);
        Node* parentNode = find(parentNodeId);

        if (parentNode != nullptr) {
            Node* startNode = parentNode;
            Node* endNode   = childNode;

            if (nodeChildTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
                startNode = childNode;
                endNode   = parentNode;
            }

            endNode->nodeColour = startNode->nodeColour;
            canvas.arrowManager.connect(startNode, endNode);
            canvas.arrowManager.refreshFor(endNode);
        }
    }

    if (!canvas.gridOriginSet && nodeChildTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        NodePosition pos = applicationContext.valueTreeState->getNodePosition(nodeId);
        canvas.gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        canvas.gridSpacing   = 50.0f;
        canvas.gridOriginSet = true;
    }

    applicationContext.rtGraphBuilder->makeRTGraph(nodeChildTree);
}

void NodeManager::remove(int nodeId)
{
    Node* node = find(nodeId);
    if (node == nullptr) {
        return;
    }

    canvas.danglingArrowLayer.removeForNode(node);
    canvas.arrowManager.removeForNode(node);
    canvas.removeChildComponent(node);
    delete node;
    nodes.erase(nodeId);
}

void NodeManager::clear()
{
    for (auto& [nodeId, node] : nodes) {
        canvas.removeChildComponent(node);
        delete node;
    }
    nodes.clear();
}

void NodeManager::setPosition(int nodeId)
{
    Node* node = find(nodeId);
    if (node == nullptr) {
        return;
    }

    juce::ValueTree nodeValueTree = applicationContext.valueTreeState->getNode(nodeId);
    if (!nodeValueTree.isValid()) {
        return;
    }

    NodePosition nodePosition = applicationContext.valueTreeState->getNodePosition(nodeId);

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;
    int radius    = nodePosition.radius;
    int height    = radius * 2;

    if (node->nodeType == NodeType::Root) {
        const int rw = RootNode::loopLimitRectangleWidth;
        node->setSize(radius * 2 + rw, height);
        node->setTopLeftPosition(xPosition - radius - rw, yPosition - radius);
    }
    else if (node->nodeType == NodeType::TraversalFlag) {
        node->setSize(radius * 4, radius * 4);
        node->setCentrePosition(xPosition, yPosition);
    }
    else {
        node->setSize(radius * 2, radius * 2);
        node->setCentrePosition(xPosition, yPosition);
    }

    canvas.arrowManager.refreshFor(node);
}

static std::unordered_set<int> collectAncestorIds(const juce::ValueTree& nodeMap, int nodeId)
{
    std::unordered_set<int> ancestors;
    std::vector<int> frontier { nodeId };

    while (! frontier.empty()) {
        int current = frontier.back();
        frontier.pop_back();

        for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
            juce::ValueTree candidate = nodeMap.getChild(i);
            juce::ValueTree candidateChildren = candidate.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

            if (! candidateChildren.getChildWithProperty(ValueTreeIdentifiers::Id, current).isValid()) {
                continue;
            }

            int parentId = candidate.getProperty(ValueTreeIdentifiers::Id);
            if (parentId == nodeId) {
                continue;
            }

            if (ancestors.insert(parentId).second) {
                frontier.push_back(parentId);
            }
        }
    }

    return ancestors;
}

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY)
{
    int rootId = (int) nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    std::unordered_set<int> visited = collectAncestorIds(applicationContext.valueTreeState->nodeMap, rootId);
    visited.insert(rootId);

    moveDescendants(nodeValueTree, deltaX, deltaY, visited);
}

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY, std::unordered_set<int>& visited)
{
    juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
        juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
        int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

        if (! visited.insert(childId).second) {
            continue;
        }

        juce::ValueTree childNodeTree = applicationContext.valueTreeState->getNode(childId);

        NodePosition childPosition = applicationContext.valueTreeState->getNodePosition(childId);
        childPosition.xPosition += deltaX;
        childPosition.yPosition += deltaY;

        applicationContext.valueTreeState->setNodePosition(childNodeTree, childPosition, applicationContext.undoManager);
        moveDescendants(childNodeTree, deltaX, deltaY, visited);
    }
}

void NodeManager::setDisplayMode(NodeDisplayMode mode) const
{
    for (auto& [nodeId, node] : nodes) {
        node->setDisplayMode(mode);
    }
}

void NodeManager::equipRootTraversals() const
{
    for (auto& [nodeId, node] : nodes) {
        if (auto* rootNode = dynamic_cast<RootNode*>(node)) {
            rootNode->equipTraversals();
        }
    }
}

void NodeManager::setInterceptsClicks(bool shouldIntercept) const
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->setInterceptsMouseClicks(shouldIntercept, shouldIntercept);
        }
    }
}
