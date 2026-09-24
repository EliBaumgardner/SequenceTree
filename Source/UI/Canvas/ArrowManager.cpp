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

ArrowManager::ArrowManager(NodeCanvas& canvasRef, const ApplicationContext& context)
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

        const int startId = arrow->startNode->nodeId;
        const int endId   = arrow->endNode->nodeId;

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
    const int parentNodeId = parentNode->nodeId;
    const int childNodeId  = childNode->nodeId;

    auto arrow = std::make_unique<Arrow>(parentNode, childNode, applicationContext);

    arrow->arrowTree = connectionTreeFor(parentNodeId, childNodeId);

    if (parentNode->nodeType == NodeType::TraversalFlag) {
        arrow->sourceHovered = parentNode->isHovered;
        arrow->initHoverState(parentNode->isHovered);
    }

    attach(*arrow);
    arrow->setInterceptsMouseClicks(false, true);

    Arrow* const raw = arrow.release();
    adopt(raw);
    return raw;
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

    const int endNodeId = endNode->nodeId;
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
        return (-((arrow.danglingIndex) + 1));
    }

    return arrow.endNode->nodeId;
}

void ArrowManager::attach(Arrow& arrow)
{
    canvas.addAndMakeVisible(arrow);
    arrow.toBack();
}

void ArrowManager::detach(Arrow* arrow)
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

    ArrowInfo arrowInfo = currentArrowInfo;

    const int nodeId = node->nodeValueTree.getProperty(ValueTreeIdentifiers::Id);

    applicationContext.graphState->arrows.applyNodeBinding(arrowInfo, nodeId);

    NodeFactory::createDanglingArrow(*applicationContext.graphState, node->nodeValueTree, tipOffset,
                                     arrowInfo, applicationContext.undoManager);
}

void ArrowManager::cancelPreview()
{
    preview.reset();
}

Node* ArrowManager::previewStartNode() const
{
    if (preview == nullptr) {
        return nullptr;
    }

    return preview->startNode;
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
        arrow->setVisible(! node->isEncapsulated || node->isEncapsulationExit);
        arrow->setArrowBounds();
        adopt(arrow.release());
    }
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
    Node* parentNode = canvas.nodeManager.find(parentNodeId);
    Node* childNode  = canvas.nodeManager.find(childNodeId);

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

void ArrowManager::handleArrowRemoved(int parentNodeId, int childNodeId)
{
    Arrow* target = find(parentNodeId, childNodeId);

    if (target == nullptr) {
        return;
    }

    remove(target);
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

    snapGhostArrow = std::make_unique<Arrow>(from, to, applicationContext);
    snapGhostArrow->isGhost = true;
    snapGhostArrow->setInterceptsMouseClicks(false, false);

    attach(*snapGhostArrow);

    snapGhostArrow->setArrowBounds();
    snapGhostArrow->triggerSnapAnimation();
}

void ArrowManager::hideSnapGhost()
{
    if (snapGhostArrow != nullptr) {
        canvas.removeChildComponent(snapGhostArrow.get());
        snapGhostArrow.reset();
    }
}
