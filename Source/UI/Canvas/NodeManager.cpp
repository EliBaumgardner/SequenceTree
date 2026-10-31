#include "NodeManager.h"

#include "EditCanvas.h"
#include "ArrowManager.h"
#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Input/CanvasDocument.h"
#include "../../Graph/ValueTreeIdentifiers.h"

NodeManager::NodeManager(EditCanvas& canvas) : canvas(canvas)
{
}

NodeManager::~NodeManager() = default;
Node* NodeManager::find(int nodeId) const
{
    const auto entry = nodes.find(nodeId);

    if (entry == nodes.end()) {
        return nullptr;
    }

    return entry->second.get();
}

Node* NodeManager::instantiateFromTree(const juce::ValueTree& nodeValueTree)
{
    jassert(nodeValueTree.isValid());

    const int nodeId = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    std::unique_ptr<Node> node = canvas.makeNode(nodeValueTree);

    jassert(node);

    if (node == nullptr) {
        return nullptr;
    }

    Node* const createdNode = node.get();

    createdNode->nodeId        = nodeId;
    createdNode->nodeValueTree = nodeValueTree;

    createdNode->onSelected = [this](Node* selectedNode, bool isSelected) {
        for (auto& listener : nodeSelectedListeners) {
            listener(selectedNode, isSelected);
        }
    };

    canvas.nodeCreated(*createdNode);

    createdNode->setMouseCursor(juce::MouseCursor::ParentCursor);

    canvas.addAndMakeVisible(createdNode);

    nodes[nodeId] = std::move(node);

    setPosition(nodeId);

    return createdNode;
}

void NodeManager::add(int nodeId)
{
    const juce::ValueTree nodeChildTree = canvas.document.findNode(nodeId);

    jassert(nodeChildTree.isValid());

    Node* const childNode = instantiateFromTree(nodeChildTree);

    if (childNode == nullptr) {
        return;
    }

    connectIncomingArrows(nodeId, childNode);
    connectOutgoingArrows(nodeChildTree, childNode);

    canvas.nodeAdded(*childNode);
}

void NodeManager::remove(int nodeId)
{
    Node* const node = find(nodeId);

    if (node == nullptr) {
        return;
    }

    canvas.nodeRemoving(*node);

    canvas.arrowManager.removeForNode(node);
    canvas.removeChildComponent(node);
    nodes.erase(nodeId);
}

void NodeManager::clear()
{
    for (auto& [nodeId, node] : nodes) {
        canvas.removeChildComponent(node.get());
    }

    nodes.clear();
}

void NodeManager::setPosition(int nodeId)
{
    Node* const           node     = find(nodeId);
    const juce::ValueTree nodeTree = canvas.document.findNode(nodeId);

    if (node == nullptr || ! nodeTree.isValid()) {
        return;
    }

    const juce::Point<int> position = canvas.drawnPosition(nodeTree);

    const int radius = ArsNovaUI::Theme::nodeRadius;

    if (node->nodeType == NodeInfo::Type::Root) {
        const int loopLimitWidth = RootNode::loopLimitRectangleWidth;

        node->setSize(radius * 2 + loopLimitWidth, radius * 2);
        node->setTopLeftPosition(position.x - radius - loopLimitWidth, position.y - radius);
    }
    else if (node->nodeType == NodeInfo::Type::TraversalFlag) {
        node->setBounds(juce::Rectangle<int>(radius * 4, radius * 4).withCentre(position));
    }
    else {
        node->setBounds(juce::Rectangle<int>(radius * 2, radius * 2).withCentre(position));
    }

    canvas.arrowManager.refreshFor(node);
}
void NodeManager::setDisplayMode(NodeInfo::DisplayMode mode)
{
    displayMode = mode;

    for (auto& [nodeId, node] : nodes) {
        node->setDisplayMode(mode);
    }
}

void NodeManager::clearHighlights()
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->setHighlightVisual(-1, false, juce::Colours::white);
        }
    }
}

void NodeManager::clearOutlines()
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->isOutlined = false;

            node->repaint();
        }
    }
}

void NodeManager::equipRootTraversals()
{
    for (auto& [nodeId, node] : nodes) {
        if (auto* rootNode = dynamic_cast<RootNode*>(node.get())) {
            rootNode->equipTraversals();
        }
    }
}

void NodeManager::setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept)
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->setInterceptsMouseClicks(shouldIntercept, shouldChildrenIntercept);
        }
    }
}

void NodeManager::selectAll() const
{
    for (auto& [nodeId, node] : nodes) {
        if (node->isVisible() && ! node->isSelected) {
            node->setSelectVisual(true);
        }
    }
}

void NodeManager::clearAll() const
{
    for (auto& [nodeId, node] : nodes) {
        if (node->isSelected) {
            node->setSelectVisual(false);
        }
    }
}

void NodeManager::deselectAllExcept(const Node& keptNode) const
{
    for (auto& [nodeId, node] : nodes) {
        if (node.get() != &keptNode && node->isSelected) {
            node->setSelectVisual(false);
        }
    }
}

void NodeManager::connectIncomingArrows(int nodeId, Node* node)
{
    const juce::ValueTree nodeMapTree = canvas.document.nodes;

    for (int parentIndex = 0; parentIndex < nodeMapTree.getNumChildren(); ++parentIndex) {
        const juce::ValueTree parentTree        = nodeMapTree.getChild(parentIndex);
        const juce::ValueTree parentChildrenIds = parentTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

        if (! parentChildrenIds.getChildWithProperty(ValueTreeIdentifiers::Id, nodeId).isValid()) {
            continue;
        }

        const int   parentNodeId = parentTree.getProperty(ValueTreeIdentifiers::Id);
        Node* const parentNode   = find(parentNodeId);

        if (parentNode == nullptr || parentNodeId == nodeId) {
            continue;
        }

        canvas.arrowManager.connectParentToChild(parentNode, node);
    }
}

void NodeManager::connectOutgoingArrows(const juce::ValueTree& nodeValueTree, Node* node)
{
    const int             nodeId          = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);
    const juce::ValueTree nodeChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int childIndex = 0; childIndex < nodeChildrenIds.getNumChildren(); ++childIndex) {
        const int   childNodeId = nodeChildrenIds.getChild(childIndex).getProperty(ValueTreeIdentifiers::Id);
        Node* const childNode   = find(childNodeId);

        if (childNode == nullptr || childNodeId == nodeId) {
            continue;
        }

        canvas.arrowManager.connectParentToChild(node, childNode);
    }
}

