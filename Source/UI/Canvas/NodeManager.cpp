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
#include "../Node/Encapsulator.h"
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
    else if (treeType == ValueTreeIdentifiers::EncapsulatorData) {
        node = std::make_unique<Encapsulator>(applicationContext);
    }

    jassert(node);

    const juce::ValueTree midiNotes = nodeValueTree.getChildWithName(ValueTreeIdentifiers::MidiNotesData);

    node->setComponentID(std::to_string(nodeId));
    node->nodeValueTree = nodeValueTree;
    node->midiNoteData  = midiNotes.getChildWithName(ValueTreeIdentifiers::MidiNoteData);
    node->setDisplayMode(displayMode);

    if (auto* const encapsulator = dynamic_cast<Encapsulator*>(node.get())) {
        encapsulator->bindToEncapsulatedNodes();
    }

    node->onSelected = [this](Node* n, bool sel) {
        for (auto& listener : nodeSelectedListeners) {
            listener(n, sel);
        }
    };

    canvas.addAndMakeVisible(node.get());
    node->setInterceptsMouseClicks(!canvas.paintMode, !canvas.paintMode && !canvas.spanMode);
    node->setMouseCursor(juce::MouseCursor::ParentCursor);

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

    if (nodeChildTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        collapseEncapsulation(nodeId);
    }

    const int owningEncapsulatorId = nodeChildTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    if (auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(find(owningEncapsulatorId))) {
        if (owningEncapsulator->isExpanded) {
            owningEncapsulator->bindToEncapsulatedNodes();
        }
        else {
            collapseEncapsulation(owningEncapsulatorId);
        }
    }

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

    if (auto* const encapsulator = dynamic_cast<Encapsulator*>(node)) {
        showEncapsulatedNodes(encapsulator->memberNodeIds);
    }

    const int encapsulatorId = node->nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    canvas.arrowManager.removeForNode(node);
    canvas.removeChildComponent(node);
    delete node;
    nodes.erase(nodeId);

    if (auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(find(encapsulatorId))) {
        owningEncapsulator->bindToEncapsulatedNodes();
    }
}

void NodeManager::clear()
{
    for (auto& [nodeId, node] : nodes) {
        canvas.removeChildComponent(node);
        delete node;
    }
    nodes.clear();
}

juce::Point<int> NodeManager::collapsedSpanShift(int nodeId) const
{
    const GraphState& graphState = *applicationContext.graphState;

    const juce::ValueTree node = graphState.getNode(nodeId);

    int walkStartId       = nodeId;
    int ownEncapsulatorId = node.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

    if (node.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        const juce::ValueTree encapsulatedIds = node.getChildWithName(ValueTreeIdentifiers::EncapsulatedIds);

        if (encapsulatedIds.getNumChildren() == 0) {
            return {};
        }

        walkStartId       = encapsulatedIds.getChild(0).getProperty(ValueTreeIdentifiers::Id);
        ownEncapsulatorId = nodeId;
    }

    juce::Point<int> shift;

    std::unordered_set<int> visited { walkStartId };
    std::unordered_set<int> shiftedEncapsulatorIds;
    std::vector<int>        frontier { walkStartId };

    while (! frontier.empty()) {
        std::vector<int> nextFrontier;

        for (const int currentId : frontier) {
            const auto parents = graphState.parentIdsOf.find(currentId);

            if (parents == graphState.parentIdsOf.end()) {
                continue;
            }

            for (const int parentId : parents->second) {
                if (! visited.insert(parentId).second) {
                    continue;
                }

                nextFrontier.push_back(parentId);

                const int encapsulatorId = graphState.getNode(parentId)
                                                     .getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);

                if (encapsulatorId < 0 || encapsulatorId == ownEncapsulatorId) {
                    continue;
                }

                const juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

                if (! encapsulator.isValid() || ! shiftedEncapsulatorIds.insert(encapsulatorId).second) {
                    continue;
                }

                auto* const encapsulatorNode = dynamic_cast<Encapsulator*>(find(encapsulatorId));

                if (encapsulatorNode != nullptr && encapsulatorNode->isExpanded) {
                    continue;
                }

                const NodePosition exitPosition         = graphState.getNodePosition(parentId);
                const NodePosition encapsulatorPosition = graphState.getNodePosition(encapsulatorId);

                shift.x -= exitPosition.xPosition - encapsulatorPosition.xPosition;
                shift.y -= exitPosition.yPosition - encapsulatorPosition.yPosition;
            }
        }

        frontier = std::move(nextFrontier);
    }

    return shift;
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

    int xPosition = nodePosition.xPosition;
    int yPosition = nodePosition.yPosition;

    const int radius = nodePosition.radius;
    const int height = radius * 2;

    const int encapsulatorId = nodeValueTree.getProperty(ValueTreeIdentifiers::EncapsulatorId, -1);
    const juce::ValueTree encapsulator = applicationContext.graphState->getNode(encapsulatorId);

    auto* const owningEncapsulator = dynamic_cast<Encapsulator*>(find(encapsulatorId));

    const bool isDrawnAtOwner = encapsulator.isValid()
                             && (owningEncapsulator == nullptr || ! owningEncapsulator->isExpanded);

    if (isDrawnAtOwner) {
        xPosition = encapsulator.getProperty(ValueTreeIdentifiers::XPosition);
        yPosition = encapsulator.getProperty(ValueTreeIdentifiers::YPosition);
    }

    const juce::Point<int> collapseShift = collapsedSpanShift(nodeId);

    xPosition += collapseShift.x;
    yPosition += collapseShift.y;

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

    applicationContext.graphState->moveEncapsulatorWithEntryMember(rootId, rootId, deltaX, deltaY,
                                                                   applicationContext.undoManager);

    moveDescendants(nodeValueTree, deltaX, deltaY, visited, rootId);
}

void NodeManager::moveDescendants(juce::ValueTree nodeValueTree, int deltaX, int deltaY,
                                  std::unordered_set<int>& visited, int draggedNodeId) const
{
    juce::Identifier childIdListType = ValueTreeIdentifiers::NodeChildrenIds;

    if (nodeValueTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        childIdListType = ValueTreeIdentifiers::EncapsulatedIds;
    }

    const juce::ValueTree nodeValueTreeChildren = nodeValueTree.getChildWithName(childIdListType);

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

        applicationContext.graphState->moveEncapsulatorWithEntryMember(childId, draggedNodeId, deltaX, deltaY,
                                                                       applicationContext.undoManager);

        moveDescendants(childNodeTree, deltaX, deltaY, visited, draggedNodeId);
    }
}

void NodeManager::collapseEncapsulation(int encapsulatorId) const
{
    auto* const encapsulator = dynamic_cast<Encapsulator*>(find(encapsulatorId));

    if (encapsulator == nullptr) {
        return;
    }

    encapsulator->isExpanded = false;

    encapsulator->bindToEncapsulatedNodes();

    encapsulator->setVisible(true);
    encapsulator->setInterceptsMouseClicks(!canvas.paintMode, !canvas.paintMode && !canvas.spanMode);

    for (const int memberNodeId : encapsulator->memberNodeIds) {
        Node* const member = find(memberNodeId);

        if (member == nullptr) {
            continue;
        }

        member->isEncapsulated         = true;
        member->isOutlined             = false;
        member->isEncapsulationRinged  = false;
        member->isEncapsulationEntry   = false;
        member->setVisible(false);
        member->setInterceptsMouseClicks(false, false);
    }

    for (const auto& [positionedNodeId, positionedNode] : nodes) {
        setPosition(positionedNodeId);
    }

    encapsulator->syncHighlightsFromMembers();

    encapsulator->toFront(false);

    canvas.arrowManager.refreshEncapsulatedArrows();
}

void NodeManager::expandEncapsulation(int encapsulatorId) const
{
    auto* const encapsulator = dynamic_cast<Encapsulator*>(find(encapsulatorId));

    if (encapsulator == nullptr || encapsulator->memberNodeIds.empty()) {
        return;
    }

    encapsulator->isExpanded = true;
    encapsulator->setVisible(false);
    encapsulator->setInterceptsMouseClicks(false, false);

    showEncapsulatedNodes(encapsulator->memberNodeIds);

    encapsulator->bindToEncapsulatedNodes();
}

void NodeManager::showEncapsulatedNodes(const std::vector<int>& memberNodeIds) const
{
    for (const int memberNodeId : memberNodeIds) {
        Node* const member = find(memberNodeId);

        if (member == nullptr) {
            continue;
        }

        member->isEncapsulated        = false;
        member->isEncapsulationRinged = false;
        member->isEncapsulationEntry  = false;
        member->setVisible(true);
        member->setInterceptsMouseClicks(!canvas.paintMode, !canvas.paintMode && !canvas.spanMode);
    }

    for (const auto& [positionedNodeId, positionedNode] : nodes) {
        setPosition(positionedNodeId);
    }

    canvas.arrowManager.refreshEncapsulatedArrows();
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

void NodeManager::syncEncapsulationHighlights() const
{
    for (auto& [nodeId, node] : nodes) {
        if (auto* const encapsulator = dynamic_cast<Encapsulator*>(node)) {
            encapsulator->syncHighlightsFromMembers();
        }
    }
}

void NodeManager::clearOutlines() const
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->isOutlined = false;
            node->repaint();
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

void NodeManager::setInterceptsClicks(bool shouldIntercept, bool shouldChildrenIntercept) const
{
    for (auto& [nodeId, node] : nodes) {
        if (node != nullptr) {
            node->setInterceptsMouseClicks(shouldIntercept, shouldChildrenIntercept);
        }
    }
}
