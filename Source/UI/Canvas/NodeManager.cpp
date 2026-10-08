#include "NodeManager.h"

#include "NodeCanvas.h"
#include "ArrowManager.h"
#include "../Node/Node.h"
#include "../Node/RootNode.h"
#include "../Node/Modulator.h"
#include "../Node/TraversalFlagNode.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Node/Encapsulator.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"

NodeManager::NodeManager(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas), graphState(graphState), undoManager(undoManager)
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

    const juce::Identifier treeType  = nodeValueTree.getType();
    const juce::ValueTree  midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);
    const int              nodeId    = nodeValueTree.getProperty(ValueTreeIdentifiers::Id);
    std::unique_ptr<Node>  node;

    if (treeType == ValueTreeIdentifiers::RootNodeData) {
        node = std::make_unique<RootNode>(undoManager, graphState);
    }
    else if (treeType == ValueTreeIdentifiers::NodeData) {
        node = std::make_unique<Node>(undoManager);
    }
    else if (treeType == ValueTreeIdentifiers::AlternativeNodeData) {
        node = std::make_unique<Node>(undoManager);
        node->isAlternativeNode = true;
    }
    else if (treeType == ValueTreeIdentifiers::TraversalFlagData) {
        node = std::make_unique<TraversalFlagNode>(undoManager, graphState);
    }
    else if (treeType == ValueTreeIdentifiers::ModulatorData
          || treeType == ValueTreeIdentifiers::ModulatorRootData) {
        node = std::make_unique<Modulator>(undoManager);
    }
    else if (treeType == ValueTreeIdentifiers::AlternativeModulatorData) {
        node = std::make_unique<Modulator>(undoManager);
        node->isAlternativeNode = true;
    }
    else if (treeType == ValueTreeIdentifiers::EncapsulatorData) {
        node = std::make_unique<Encapsulator>(undoManager, graphState, nodeCanvas);
    }

    jassert(node);

    if (node == nullptr) {
        return nullptr;
    }

    Node* const createdNode = node.get();

    createdNode->nodeId        = nodeId;
    createdNode->nodeValueTree = nodeValueTree;
    createdNode->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    createdNode->nodeColour    = juce::Colour::fromString(nodeValueTree.getProperty(ValueTreeIdentifiers::NodeColour, CustomLookAndFeel::get(nodeCanvas).defaultNodeColour.toString()).toString());

    createdNode->onSelected = [this](Node* selectedNode, bool isSelected) {
        for (auto& listener : nodeSelectedListeners) {
            listener(selectedNode, isSelected);
        }
    };

    createdNode->bindToTree();
    createdNode->setDisplayMode(displayMode);
    createdNode->setInterceptsMouseClicks(!nodeCanvas.paintMode, !nodeCanvas.paintMode && !nodeCanvas.spanMode);
    createdNode->setMouseCursor(juce::MouseCursor::ParentCursor);

    nodeCanvas.addAndMakeVisible(createdNode);

    nodes[nodeId] = std::move(node);

    setPosition(nodeId);

    return createdNode;
}

void NodeManager::setPosition(int nodeId)
{
    Node* const           node          = find(nodeId);
    const juce::ValueTree nodeValueTree = graphState.getNode(nodeId);

    if (node == nullptr || !nodeValueTree.isValid()) {
        return;
    }

    const NodePosition     nodePosition       = graphState.getNodePosition(nodeId);
    const int              radius             = Theme::nodeRadius;
    const int              encapsulatorId     = nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);
    const juce::ValueTree  encapsulator       = graphState.getNode(encapsulatorId);
    auto* const            owningEncapsulator = dynamic_cast<Encapsulator*>(find(encapsulatorId));
    const juce::Point<int> collapseShift      = nodeCanvas.encapsulationView.collapsedSpanShift(nodeId);
    const bool             isDrawnAtOwner     = encapsulator.isValid()
                                             && (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded);
    int                    xPosition          = nodePosition.xPosition;
    int                    yPosition          = nodePosition.yPosition;

    if (isDrawnAtOwner) {
        xPosition = encapsulator.getProperty(ValueTreeIdentifiers::XPosition);
        yPosition = encapsulator.getProperty(ValueTreeIdentifiers::YPosition);
    }

    xPosition += collapseShift.x;
    yPosition += collapseShift.y;

    if (node->nodeType == NodeType::Root) {
        const int loopLimitWidth = RootNode::loopLimitRectangleWidth;

        node->setSize(radius * 2 + loopLimitWidth, radius * 2);
        node->setTopLeftPosition(xPosition - radius - loopLimitWidth, yPosition - radius);
    }
    else if (node->nodeType == NodeType::TraversalFlag) {
        node->setBounds(juce::Rectangle<int>(radius * 4, radius * 4).withCentre({ xPosition, yPosition }));
    }
    else {
        node->setBounds(juce::Rectangle<int>(radius * 2, radius * 2).withCentre({ xPosition, yPosition }));
    }

    nodeCanvas.arrowManager.refreshFor(node);
}

void NodeManager::add(int nodeId)
{
    const juce::ValueTree nodeChildTree = graphState.getNode(nodeId);

    jassert(nodeChildTree.isValid());

    Node* const childNode = instantiateFromTree(nodeChildTree);

    if (childNode == nullptr) {
        return;
    }

    connectIncomingArrows(nodeId, childNode);
    connectOutgoingArrows(nodeChildTree, childNode);

    if (nodeChildTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        nodeCanvas.encapsulationView.collapse(nodeId);
    }

    const int owningEncapsulatorId = nodeChildTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    if (auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(find(owningEncapsulatorId))) {
        if (owningEncapsulator->isExpanded) {
            nodeCanvas.encapsulationView.refreshMembership(owningEncapsulatorId);
        }
        else {
            nodeCanvas.encapsulationView.collapse(owningEncapsulatorId);
        }
    }

    if (!nodeCanvas.gridOriginSet && nodeChildTree.getType() == ValueTreeIdentifiers::RootNodeData) {
        const NodePosition rootPosition = graphState.getNodePosition(nodeId);

        nodeCanvas.gridOrigin    = { static_cast<float>(rootPosition.xPosition), static_cast<float>(rootPosition.yPosition) };
        nodeCanvas.gridSpacing   = ArrowInfo::pixelsPerGridSpace;
        nodeCanvas.gridOriginSet = true;
    }
}

void NodeManager::connectIncomingArrows(int nodeId, Node* node)
{
    const juce::ValueTree nodeMapTree = graphState.nodeMap;

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

        nodeCanvas.arrowManager.connectParentToChild(parentNode, node);
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

        nodeCanvas.arrowManager.connectParentToChild(node, childNode);
    }
}

void NodeManager::remove(int nodeId)
{
    Node* const node = find(nodeId);

    if (node == nullptr) {
        return;
    }

    if (auto* const encapsulator = dynamic_cast<Encapsulator*>(node)) {
        nodeCanvas.encapsulationView.showMembers(encapsulator->memberNodeIds);
    }

    const int encapsulatorId = node->nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    nodeCanvas.arrowManager.removeForNode(node);
    nodeCanvas.removeChildComponent(node);
    nodes.erase(nodeId);

    nodeCanvas.encapsulationView.refreshMembership(encapsulatorId);
}

void NodeManager::clear()
{
    for (auto& [nodeId, node] : nodes) {
        nodeCanvas.removeChildComponent(node.get());
    }

    nodes.clear();
}

static std::unordered_set<int> collectAncestorIds(const GraphState& graphState, int nodeId)
{
    std::unordered_set<int> ancestors;
    std::vector<int>        frontier { nodeId };

    while (! frontier.empty()) {
        const int currentId = frontier.back();

        frontier.pop_back();

        const auto parents = graphState.parentIdsOf.find(currentId);

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

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY)
{
    const int               rootId  = static_cast<int>(nodeValueTree.getProperty(ValueTreeIdentifiers::Id));
    std::unordered_set<int> visited = collectAncestorIds(graphState, rootId);

    visited.insert(rootId);

    moveEncapsulatorWithEntryMember(rootId, rootId, deltaX, deltaY);

    moveDescendants(nodeValueTree, deltaX, deltaY, visited, rootId);
}

void NodeManager::moveEncapsulatorWithEntryMember(int nodeId, int draggedNodeId, int deltaX, int deltaY)
{
    const int              encapsulatorId = graphState.getNode(nodeId).getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);
    const juce::ValueTree  encapsulator   = graphState.getNode(encapsulatorId);
    const std::vector<int> memberNodeIds  = graphState.encapsulation.memberIds(encapsulatorId);

    if (memberNodeIds.empty() || encapsulatorId == draggedNodeId || memberNodeIds.front() != nodeId) {
        return;
    }

    NodePosition encapsulatorPosition = graphState.getNodePosition(encapsulatorId);

    encapsulatorPosition.xPosition += deltaX;
    encapsulatorPosition.yPosition += deltaY;

    graphState.setNodePosition(encapsulator, encapsulatorPosition, &undoManager);
}

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY,
                                  std::unordered_set<int>& visited, int draggedNodeId)
{
    juce::Identifier childIdListType = ValueTreeIdentifiers::NodeChildrenIds;

    if (nodeValueTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        childIdListType = ValueTreeIdentifiers::EncapsulatedIds;
    }

    const juce::ValueTree childIds = nodeValueTree.getChildWithName(childIdListType);

    for (int childIndex = 0; childIndex < childIds.getNumChildren(); ++childIndex) {
        const int childId = childIds.getChild(childIndex).getProperty(ValueTreeIdentifiers::Id);

        if (! visited.insert(childId).second) {
            continue;
        }

        const juce::ValueTree childNodeTree = graphState.getNode(childId);
        NodePosition          childPosition = graphState.getNodePosition(childId);

        childPosition.xPosition += deltaX;
        childPosition.yPosition += deltaY;

        graphState.setNodePosition(childNodeTree, childPosition, &undoManager);

        moveEncapsulatorWithEntryMember(childId, draggedNodeId, deltaX, deltaY);

        moveDescendants(childNodeTree, deltaX, deltaY, visited, draggedNodeId);
    }
}

void NodeManager::setDisplayMode(NodeDisplayMode mode)
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
