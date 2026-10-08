#include "ArrowManager.h"

#include "NodeCanvas.h"
#include "NodeManager.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Graph/NodeFactory.h"

ArrowManager::ArrowManager(NodeCanvas& nodeCanvas, GraphState& graphState, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas), graphState(graphState), undoManager(undoManager)
{
}

ArrowManager::~ArrowManager() = default;

Arrow* ArrowManager::find(int parentNodeId, int childNodeId) const
{
    for (Arrow* const arrow : arrows) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }

        const int startId = arrow->startNode->nodeId;
        const int endId   = arrow->endNode->nodeId;

        if ((startId == parentNodeId && endId == childNodeId)
            || (startId == childNodeId && endId == parentNodeId)) {
            return arrow;
        }
    }

    return nullptr;
}

Arrow* ArrowManager::connect(Node* parentNode, Node* childNode)
{
    auto         arrow        = std::make_unique<Arrow>(parentNode, childNode, undoManager);
    Arrow* const createdArrow = arrow.get();

    createdArrow->arrowTree = connectionTreeFor(parentNode->nodeId, childNode->nodeId);

    if (parentNode->nodeType == NodeType::TraversalFlag) {
        createdArrow->sourceHovered = parentNode->isHovered;

        createdArrow->initHoverState(parentNode->isHovered);
    }

    nodeCanvas.addAndMakeVisible(*createdArrow);

    createdArrow->toBack();
    createdArrow->setInterceptsMouseClicks(false, true);

    adopt(std::move(arrow));

    return createdArrow;
}

juce::ValueTree ArrowManager::connectionTreeFor(int startNodeId, int endNodeId) const
{
    const juce::ValueTree connection = graphState.getConnection(startNodeId, endNodeId);

    if (connection.isValid()) {
        return connection;
    }

    return graphState.getConnection(endNodeId, startNodeId);
}

void ArrowManager::adopt(std::unique_ptr<Arrow> arrow)
{
    if (arrow == nullptr) {
        return;
    }

    if (arrow->startNode != nullptr) {
        arrow->startNode->nodeArrows.insert({ arrowKey(*arrow), arrow.get() });
    }

    arrows.add(arrow.release());
}

int ArrowManager::arrowKey(const Arrow& arrow)
{
    if (arrow.isDangling()) {
        return -(arrow.danglingIndex + 1);
    }

    return arrow.endNode->nodeId;
}

Arrow* ArrowManager::connectParentToChild(Node* parentNode, Node* childNode)
{
    Node* startNode = parentNode;
    Node* endNode   = childNode;

    if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData
        || childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeModulatorData) {
        startNode = childNode;
        endNode   = parentNode;
    }

    if (startNode->nodeArrows.count(endNode->nodeId) > 0) {
        return nullptr;
    }

    Arrow* const arrow = connect(startNode, endNode);

    refreshFor(endNode);

    return arrow;
}

void ArrowManager::refreshFor(const Node* movedNode)
{
    for (Arrow* const arrow : arrows) {
        Node* const parentNode = arrow->startNode;
        Node* const childNode  = arrow->endNode;

        if (parentNode != movedNode && childNode != movedNode) {
            continue;
        }

        if (! arrow->isDangling()) {
            parentNode->refreshValueDisplay();
            childNode->refreshValueDisplay();
        }

        arrow->setArrowBounds();
    }
}

void ArrowManager::remove(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    const int index = arrows.indexOf(arrow);

    if (index >= 0) {
        detach(arrow);
        arrows.remove(index);
    }
}

void ArrowManager::detach(Arrow* arrow)
{
    if (arrow->startNode != nullptr) {
        auto&      nodeArrows = arrow->startNode->nodeArrows;
        const auto arrowRange = nodeArrows.equal_range(arrowKey(*arrow));

        for (auto entry = arrowRange.first; entry != arrowRange.second; ++entry) {
            if (entry->second == arrow) {
                nodeArrows.erase(entry);
                break;
            }
        }
    }

    nodeCanvas.removeChildComponent(arrow);
}

void ArrowManager::removeForNode(const Node* node)
{
    if (preview != nullptr && preview->startNode == node) {
        preview.reset();
    }

    if (snapGhostArrow != nullptr
        && (snapGhostArrow->startNode == node || snapGhostArrow->endNode == node)) {
        hideSnapGhost();
    }

    removeMatching([node](Arrow* arrow) {
        return arrow->startNode == node || arrow->endNode == node;
    });
}

void ArrowManager::hideSnapGhost()
{
    if (snapGhostArrow != nullptr) {
        nodeCanvas.removeChildComponent(snapGhostArrow.get());

        snapGhostArrow.reset();
    }
}

void ArrowManager::removeMatching(const std::function<bool(Arrow*)>& predicate)
{
    for (int arrowIndex = arrows.size() - 1; arrowIndex >= 0; --arrowIndex) {
        Arrow* const arrow = arrows[arrowIndex];

        if (predicate(arrow)) {
            detach(arrow);

            arrows.remove(arrowIndex);
        }
    }
}

void ArrowManager::clear()
{
    hideSnapGhost();
    preview.reset();
    arrows.clear();
}

void ArrowManager::updatePreview(Node* node, juce::Point<int> tipOffset, bool dashed)
{
    if (node == nullptr) {
        return;
    }

    if (preview == nullptr || preview->startNode != node) {
        preview = std::make_unique<Arrow>(node, tipOffset, undoManager);

        nodeCanvas.addAndMakeVisible(*preview);

        preview->toBack();
    }

    preview->dashed = dashed;

    preview->setTipOffset(tipOffset);
}

void ArrowManager::commitPreview()
{
    if (preview == nullptr) {
        return;
    }

    const Node* const      node      = preview->startNode;
    const juce::Point<int> tipOffset = preview->tipOffset;

    preview.reset();

    if (node == nullptr) {
        return;
    }

    ArrowInfo arrowInfo = currentArrowInfo;
    const int nodeId    = node->nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    graphState.arrows.applyNodeBinding(arrowInfo, nodeId);

    NodeFactory::createDanglingArrow(graphState, node->nodeValueTree, tipOffset, arrowInfo, &undoManager);
}

void ArrowManager::rebuildDanglingForNode(int nodeId)
{
    Node* const node = nodeCanvas.nodeManager.find(nodeId);

    removeMatching([node](Arrow* arrow) {
        return arrow->isDangling() && arrow->startNode == node;
    });

    if (node == nullptr) {
        return;
    }

    const juce::ValueTree arrowList = node->nodeValueTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

    if (! arrowList.isValid()) {
        return;
    }

    for (int danglingIndex = 0; danglingIndex < arrowList.getNumChildren(); ++danglingIndex) {
        const juce::ValueTree  arrowTree = arrowList.getChild(danglingIndex);
        const juce::Point<int> tipOffset { static_cast<int>(arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX)),
                                           static_cast<int>(arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY)) };
        auto                   arrow     = std::make_unique<Arrow>(node, tipOffset, undoManager);

        arrow->arrowTree     = arrowTree;
        arrow->danglingIndex = danglingIndex;

        arrow->valueEditor->bindEditor(arrowTree, ValueTreeIdentifiers::CountLimit);

        nodeCanvas.addAndMakeVisible(*arrow);

        arrow->toBack();
        arrow->setVisible(! node->isEncapsulated || node->isEncapsulationExit);
        arrow->setArrowBounds();

        adopt(std::move(arrow));
    }
}

void ArrowManager::refreshEncapsulatedArrows()
{
    for (Arrow* const arrow : arrows) {
        if (arrow->startNode == nullptr) {
            continue;
        }

        if (arrow->isDangling()) {
            arrow->setVisible(! arrow->startNode->isEncapsulated
                              || arrow->startNode->isEncapsulationExit);
            continue;
        }

        arrow->setVisible(! (arrow->startNode->isEncapsulated && arrow->endNode->isEncapsulated));
    }
}

void ArrowManager::handleArrowAdded(int parentNodeId, int childNodeId)
{
    Node* const parentNode = nodeCanvas.nodeManager.find(parentNodeId);
    Node* const childNode  = nodeCanvas.nodeManager.find(childNodeId);

    if (parentNode == nullptr || childNode == nullptr) {
        return;
    }

    connectParentToChild(parentNode, childNode);
}

void ArrowManager::handleArrowInfoChanged(int parentNodeId, int childNodeId)
{
    Arrow* const arrow = find(parentNodeId, childNodeId);

    if (arrow != nullptr) {
        arrow->arrowTree = connectionTreeFor(parentNodeId, childNodeId);

        arrow->repaint();
    }
}

void ArrowManager::setSelected(Arrow* arrow)
{
    clearSelection();

    if (arrow != nullptr && ! arrow->selected) {
        arrow->selected = true;

        arrow->repaint();
    }
}

void ArrowManager::clearSelection()
{
    for (Arrow* const arrow : arrows) {
        if (arrow->selected) {
            arrow->selected = false;

            arrow->repaint();
        }
    }
}

void ArrowManager::resetAllProgress()
{
    for (Arrow* const arrow : arrows) {
        if (arrow != nullptr) {
            arrow->resetProgress();
        }
    }
}

void ArrowManager::pauseAllProgress()
{
    const double nowMs = juce::Time::getMillisecondCounterHiRes();

    for (Arrow* const arrow : arrows) {
        if (arrow == nullptr || arrow->animation.trailsPaused) {
            continue;
        }

        arrow->animation.trailsPaused = true;
        arrow->animation.pausedAtMs   = nowMs;
    }
}

void ArrowManager::resumeAllProgress()
{
    for (Arrow* const arrow : arrows) {
        if (arrow != nullptr) {
            arrow->resumeProgress();
        }
    }
}

void ArrowManager::resetTrail(int trailId)
{
    for (Arrow* const arrow : arrows) {
        if (arrow != nullptr) {
            arrow->resetProgress(trailId);
        }
    }
}

void ArrowManager::triggerSnapForNode(int nodeId)
{
    Node* const node = nodeCanvas.nodeManager.find(nodeId);

    if (node == nullptr) {
        return;
    }

    for (Arrow* const arrow : arrows) {
        if (arrow->endNode == node) {
            arrow->triggerSnapAnimation();
            return;
        }
    }
}

void ArrowManager::showSnapGhost(Node* from, Node* to)
{
    if (snapGhostArrow != nullptr
        && snapGhostArrow->startNode == from && snapGhostArrow->endNode == to) {
        return;
    }

    hideSnapGhost();

    snapGhostArrow = std::make_unique<Arrow>(from, to, undoManager);

    snapGhostArrow->isGhost = true;

    nodeCanvas.addAndMakeVisible(*snapGhostArrow);

    snapGhostArrow->toBack();
    snapGhostArrow->setInterceptsMouseClicks(false, false);
    snapGhostArrow->setArrowBounds();
    snapGhostArrow->triggerSnapAnimation();
}
