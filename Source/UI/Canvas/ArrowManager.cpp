//
// Created by Eli Baumgardner on 7/21/26.
//

#include "ArrowManager.h"

#include "NodeCanvas.h"
#include "NodeManager.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../Node/NodeFactory.h"
#include "../../Audio/AudioUIBridge.h"
#include "../../Util/ApplicationContext.h"

ArrowManager::ArrowManager(NodeCanvas& canvasRef, ApplicationContext& context)
    : canvas(canvasRef), applicationContext(context)
{
}

ArrowManager::~ArrowManager() = default;

Arrow* ArrowManager::find(int parentNodeId, int childNodeId) const
{
    for (Arrow* const arrow : arrows) {
        if (arrow->startNode == nullptr || arrow->endNode == nullptr) {
            continue;
        }

        const int startId = arrow->startNode->getComponentID().getIntValue();
        const int endId   = arrow->endNode->getComponentID().getIntValue();

        if ((startId == parentNodeId && endId == childNodeId)
            || (startId == childNodeId && endId == parentNodeId)) {
            return arrow;
        }
    }

    return nullptr;
}

juce::ValueTree ArrowManager::connectionTreeFor(int startNodeId, int endNodeId) const
{
    GraphState& state = *applicationContext.graphState;

    const juce::ValueTree connection = state.getConnection(startNodeId, endNodeId);

    if (connection.isValid()) {
        return connection;
    }

    return state.getConnection(endNodeId, startNodeId);
}

Arrow* ArrowManager::connect(Node* parentNode, Node* childNode)
{
    const int parentNodeId = parentNode->getComponentID().getIntValue();
    const int childNodeId  = childNode->getComponentID().getIntValue();

    auto arrow = std::make_unique<Arrow>(parentNode, childNode, applicationContext);

    arrow->arrowTree = connectionTreeFor(parentNodeId, childNodeId);

    if (parentNode->nodeType == NodeType::TraversalFlag) {
        arrow->sourceHovered = parentNode->isHovered;
        arrow->initHoverState(parentNode->isHovered);
    }

    attach(*arrow);
    arrow->setInterceptsMouseClicks(false, false);

    Arrow* const raw = arrow.release();
    adopt(raw);
    return raw;
}

Arrow* ArrowManager::connectParentToChild(Node* parentNode, Node* childNode)
{
    Node* startNode = parentNode;
    Node* endNode   = childNode;

    if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
        startNode = childNode;
        endNode   = parentNode;
    }

    const int endNodeId = endNode->getComponentID().getIntValue();
    if (startNode->nodeArrows.count(endNodeId) > 0) {
        return nullptr;
    }

    endNode->nodeColour = startNode->nodeColour;

    Arrow* const arrow = connect(startNode, endNode);
    refreshFor(endNode);

    return arrow;
}

void ArrowManager::adopt(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    if (arrow->startNode != nullptr) {
        arrow->startNode->nodeArrows.insert({ arrowKey(*arrow), arrow });
    }

    arrows.add(arrow);
}

int ArrowManager::arrowKey(const Arrow& arrow)
{
    if (arrow.isDangling()) {
        return AudioUIBridge::danglingArrowKey(arrow.danglingIndex);
    }

    return arrow.endNode->getComponentID().getIntValue();
}

void ArrowManager::attach(Arrow& arrow) const
{
    canvas.addAndMakeVisible(arrow);
    arrow.toBack();
}

void ArrowManager::detach(Arrow* arrow) const
{
    if (arrow->startNode != nullptr) {
        auto& nodeArrows = arrow->startNode->nodeArrows;
        const auto range = nodeArrows.equal_range(arrowKey(*arrow));

        for (auto entry = range.first; entry != range.second; ++entry) {
            if (entry->second == arrow) {
                nodeArrows.erase(entry);
                break;
            }
        }
    }

    canvas.removeChildComponent(arrow);
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

void ArrowManager::removeForNode(const Node* node)
{
    removeMatching([node](Arrow* arrow) {
        return arrow->startNode == node || arrow->endNode == node;
    });
}

void ArrowManager::removeMatching(const std::function<bool(Arrow*)>& predicate)
{
    for (int i = arrows.size() - 1; i >= 0; --i) {
        Arrow* const arrow = arrows[i];

        if (predicate(arrow)) {
            detach(arrow);
            arrows.remove(i);
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
        preview = std::make_unique<Arrow>(node, tipOffset, applicationContext);
        attach(*preview);
    }

    preview->dashed = dashed;
    preview->setTipOffset(tipOffset);
}

void ArrowManager::commitPreview()
{
    if (preview == nullptr) {
        return;
    }

    const Node* const node = preview->startNode;
    const juce::Point<int> tipOffset = preview->tipOffset;

    preview.reset();

    if (node == nullptr) {
        return;
    }

    NodeFactory::createDanglingArrow(*applicationContext.graphState, node->nodeValueTree, tipOffset,
                                     currentArrowInfo, applicationContext.undoManager);
}

void ArrowManager::cancelPreview()
{
    preview.reset();
}

Node* ArrowManager::previewStartNode() const
{
    return preview != nullptr ? preview->startNode : nullptr;
}

void ArrowManager::rebuildDanglingForNode(int nodeId)
{
    Node* const node = canvas.nodeManager.find(nodeId);

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

    for (int i = 0; i < arrowList.getNumChildren(); ++i) {
        const juce::ValueTree arrowTree = arrowList.getChild(i);

        const juce::Point<int> tipOffset {
            (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX),
            (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY)
        };

        auto arrow = std::make_unique<Arrow>(node, tipOffset, applicationContext);
        arrow->arrowTree     = arrowTree;
        arrow->danglingIndex = i;
        arrow->valueEditor->bindEditor(arrowTree, ValueTreeIdentifiers::CountLimit);
        attach(*arrow);
        arrow->setArrowBounds();
        adopt(arrow.release());
    }
}

void ArrowManager::refreshFor(const Node* movedNode) const
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

void ArrowManager::refreshEncapsulatedArrows() const
{
    for (Arrow* const arrow : arrows) {
        if (arrow->startNode == nullptr) {
            continue;
        }

        bool endEncapsulated = arrow->startNode->isEncapsulated;

        if (arrow->endNode != nullptr) {
            endEncapsulated = arrow->endNode->isEncapsulated;
        }

        arrow->setVisible(! (arrow->startNode->isEncapsulated && endEncapsulated));
    }
}

void ArrowManager::handleArrowAdded(int parentNodeId, int childNodeId)
{
    Node* parentNode = canvas.nodeManager.find(parentNodeId);
    Node* childNode  = canvas.nodeManager.find(childNodeId);

    if (parentNode == nullptr || childNode == nullptr) {
        return;
    }

    if (connectParentToChild(parentNode, childNode) == nullptr) {
        return;
    }

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.graphState->getNode(parentNodeId));
}

void ArrowManager::handleArrowTypeChanged(int parentNodeId, int childNodeId)
{
    Arrow* const arrow = find(parentNodeId, childNodeId);

    if (arrow != nullptr) {
        arrow->arrowTree = connectionTreeFor(parentNodeId, childNodeId);
        arrow->repaint();
    }

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.graphState->getNode(parentNodeId));
}

void ArrowManager::handleArrowRemoved(int parentNodeId, int childNodeId)
{
    Arrow* target = find(parentNodeId, childNodeId);

    if (target == nullptr) {
        return;
    }

    remove(target);

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.graphState->getNode(parentNodeId));
}

void ArrowManager::setSelected(Arrow* arrow) const
{
    clearSelection();

    if (arrow != nullptr && ! arrow->selected) {
        arrow->selected = true;
        arrow->repaint();
    }
}

void ArrowManager::clearSelection() const
{
    for (Arrow* const arrow : arrows) {
        if (arrow->selected) {
            arrow->selected = false;
            arrow->repaint();
        }
    }
}

void ArrowManager::resetAllProgress() const
{
    for (Arrow* const arrow : arrows) {
        if (arrow != nullptr) {
            arrow->resetProgress();
        }
    }
}

void ArrowManager::resetTrail(int trailId) const
{
    for (Arrow* const arrow : arrows) {
        if (arrow != nullptr) {
            arrow->resetProgress(trailId);
        }
    }
}

void ArrowManager::triggerSnapForNode(int nodeId) const
{
    Node* const node = canvas.nodeManager.find(nodeId);
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

    snapGhostArrow = new Arrow(from, to, applicationContext);
    snapGhostArrow->isGhost = true;
    snapGhostArrow->setInterceptsMouseClicks(false, false);

    attach(*snapGhostArrow);

    snapGhostArrow->setArrowBounds();
    snapGhostArrow->triggerSnapAnimation();
}

void ArrowManager::hideSnapGhost()
{
    if (snapGhostArrow != nullptr) {
        canvas.removeChildComponent(snapGhostArrow);
        delete snapGhostArrow;
        snapGhostArrow = nullptr;
    }
}
