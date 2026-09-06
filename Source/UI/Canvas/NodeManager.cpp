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
#include "../../Graph/GraphState.h"
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
    if (nodePair == nodes.end()) {
        return nullptr;
    }

    return nodePair->second;
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

    const juce::ValueTree midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    node->setComponentID(std::to_string(nodeId));
    node->nodeValueTree = nodeValueTree;
    node->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node->setDisplayMode(displayMode);

    node->onSelected = [this](Node* n, bool sel) {
        for (auto& listener : nodeSelectedListeners) {
            listener(n, sel);
        }
    };

    canvas.addAndMakeVisible(node.get());
    node->setInterceptsMouseClicks(!canvas.paintMode, !canvas.paintMode);

    Node* const raw = node.release();
    nodes[nodeId] = raw;

    setPosition(nodeId);

    return raw;
}

void NodeManager::connectIncomingArrows(int nodeId, Node* node) const
{
    const juce::ValueTree nodeMapTree = applicationContext.graphState->nodeMap;

    for (int i = 0; i < nodeMapTree.getNumChildren(); ++i) {
        const juce::ValueTree parentTree = nodeMapTree.getChild(i);
        const juce::ValueTree parentChildrenIds = parentTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        if (! parentChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId).isValid()) {
            continue;
        }

        const int parentNodeId = parentTree.getProperty(ValueTreeIdentifiers::Id);
        Node* const parentNode = find(parentNodeId);

        if (parentNode == nullptr || parentNodeId == nodeId) {
            continue;
        }

        canvas.arrowManager.connectParentToChild(parentNode, node);
        applicationContext.rtGraphBuilder->makeRTGraph(parentTree);
    }
}

void NodeManager::connectOutgoingArrows(const juce::ValueTree& nodeValueTree, Node* node) const
{
    const int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);
    const juce::ValueTree nodeChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < nodeChildrenIds.getNumChildren(); ++i) {
        const int childNodeId = nodeChildrenIds.getChild(i).getProperty(ValueTreeIdentifiers::Id);
        Node* const childNode = find(childNodeId);

        if (childNode == nullptr || childNodeId == nodeId) {
            continue;
        }

        canvas.arrowManager.connectParentToChild(node, childNode);
    }
}

void NodeManager::add(int nodeId)
{
    const juce::ValueTree nodeChildTree = applicationContext.graphState->getNode(nodeId);

    jassert(nodeChildTree.isValid());

    Node* const childNode = instantiateFromTree(nodeChildTree);

    connectIncomingArrows(nodeId, childNode);
    connectOutgoingArrows(nodeChildTree, childNode);

    if (!canvas.gridOriginSet && nodeChildTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        const NodePosition pos = applicationContext.graphState->getNodePosition(nodeId);
        canvas.gridOrigin    = { (float)pos.xPosition, (float)pos.yPosition };
        canvas.gridSpacing   = ArrowInfo::pixelsPerGridSpace;
        canvas.gridOriginSet = true;
    }

    applicationContext.rtGraphBuilder->makeRTGraph(nodeChildTree);
}

void NodeManager::remove(int nodeId)
{
    Node* const node = find(nodeId);
    if (node == nullptr) {
        return;
    }

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

void NodeManager::setPosition(int nodeId) const
{
    Node* const node = find(nodeId);
    if (node == nullptr) {
        return;
    }

    const juce::ValueTree nodeValueTree = applicationContext.graphState->getNode(nodeId);
    if (!nodeValueTree.isValid()) {
        return;
    }

    const NodePosition nodePosition = applicationContext.graphState->getNodePosition(nodeId);

    const int xPosition = nodePosition.xPosition;
    const int yPosition = nodePosition.yPosition;
    const int radius    = nodePosition.radius;
    const int height    = radius * 2;

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

static std::unordered_set<int> collectAncestorIds(const GraphState& graphState, int nodeId)
{
    std::unordered_set<int> ancestors;
    std::vector<int> frontier { nodeId };

    while (! frontier.empty()) {
        const int current = frontier.back();
        frontier.pop_back();

        const auto parents = graphState.parentIdsOf.find(current);

        if (parents == graphState.parentIdsOf.end()) {
            continue;
        }

        for (const int parentId : parents->second) {
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

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY) const
{
    const int rootId = (int) nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    std::unordered_set<int> visited = collectAncestorIds(*applicationContext.graphState, rootId);
    visited.insert(rootId);

    moveDescendants(nodeValueTree, deltaX, deltaY, visited);
}

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY, std::unordered_set<int>& visited) const
{
    const juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < nodeValueTreeChildren.getNumChildren(); i++) {
        const juce::ValueTree childIdTree = nodeValueTreeChildren.getChild(i);
        const int childId = childIdTree.getProperty(ValueTreeIdentifiers::Id);

        if (! visited.insert(childId).second) {
            continue;
        }

        const juce::ValueTree childNodeTree = applicationContext.graphState->getNode(childId);

        NodePosition childPosition = applicationContext.graphState->getNodePosition(childId);
        childPosition.xPosition += deltaX;
        childPosition.yPosition += deltaY;

        GraphState::setNodePosition(childNodeTree, childPosition, applicationContext.undoManager);
        moveDescendants(childNodeTree, deltaX, deltaY, visited);
    }
}

void NodeManager::setDisplayMode(NodeDisplayMode mode)
{
    displayMode = mode;

    for (auto& [nodeId, node] : nodes) {
        node->setDisplayMode(mode);
    }
}

void NodeManager::clearHighlights() const
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->setHighlightVisual(-1, false, juce::Colours::white);
        }
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
