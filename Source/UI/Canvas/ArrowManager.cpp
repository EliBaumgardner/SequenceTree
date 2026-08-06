//
// Created by Eli Baumgardner on 7/21/26.
//

#include "ArrowManager.h"

#include "NodeCanvas.h"
#include "NodeManager.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"
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
    ValueTreeState& state = *applicationContext.valueTreeState;

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

    const juce::ValueTree parentMidiNotesData = applicationContext.valueTreeState->getMidiNotes(parentNodeId);
    const juce::ValueTree parentMidiNoteData  = parentMidiNotesData.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    auto arrow = std::make_unique<Arrow>(parentNode, childNode, applicationContext);

    arrow->arrowTree = connectionTreeFor(parentNodeId, childNodeId);

    parentNode->nodeArrows[childNodeId] = arrow.get();

    if (parentNode->nodeType == NodeType::TraversalFlag) {
        arrow->sourceHovered = parentNode->isHovered;
        arrow->initHoverState(parentNode->isHovered);
    }

    attach(*arrow);
    arrow->setInterceptsMouseClicks(false, false);

    if (parentMidiNoteData.isValid() && childNode->nodeType != NodeType::Root) {
        arrow->bindToProperty(parentMidiNoteData, ValueTreeIdentifiers::MidiDuration);
    }

    Arrow* const raw = arrow.release();
    arrows.add(raw);
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
    if (arrow != nullptr) {
        arrows.add(arrow);
    }
}

void ArrowManager::attach(Arrow& arrow) const
{
    canvas.addAndMakeVisible(arrow);
    arrow.toBack();
}

void ArrowManager::detach(Arrow* arrow) const
{
    if (arrow->startNode != nullptr && arrow->endNode != nullptr) {
        const int childNodeId = arrow->endNode->getComponentID().getIntValue();
        arrow->startNode->nodeArrows.erase(childNodeId);
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
        return ! arrow->isDangling()
            && (arrow->startNode == node || arrow->endNode == node);
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
    arrows.clear();
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

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.valueTreeState->getNode(parentNodeId));
}

void ArrowManager::handleArrowTypeChanged(int parentNodeId, int childNodeId)
{
    Arrow* const arrow = find(parentNodeId, childNodeId);

    if (arrow != nullptr) {
        arrow->arrowTree = connectionTreeFor(parentNodeId, childNodeId);
        arrow->repaint();
    }

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.valueTreeState->getNode(parentNodeId));
}

void ArrowManager::handleArrowRemoved(int parentNodeId, int childNodeId)
{
    Arrow* target = find(parentNodeId, childNodeId);

    if (target == nullptr) {
        return;
    }

    remove(target);

    applicationContext.rtGraphBuilder->makeRTGraph(applicationContext.valueTreeState->getNode(parentNodeId));
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

void ArrowManager::resetGraphProgress(int graphId, int traversalId) const
{
    for (Arrow* const arrow : arrows) {
        if (arrow == nullptr || arrow->startNode == nullptr) {
            continue;
        }

        const int parentId = arrow->startNode->getComponentID().getIntValue();
        const juce::ValueTree arrowRoot = applicationContext.valueTreeState->getRootNode(parentId);
        if (! arrowRoot.isValid()) {
            continue;
        }

        if (static_cast<int>(arrowRoot.getProperty(ValueTreeIdentifiers::Id)) == graphId) {
            arrow->resetProgress(traversalId);
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
