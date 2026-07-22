//
// Created by Eli Baumgardner on 7/21/26.
//

#include "ArrowManager.h"

#include "NodeCanvas.h"
#include "NodeManager.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"
#include "../Node/NodeTextEditor.h"
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
    for (Arrow* arrow : arrows) {
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

Arrow* ArrowManager::connect(Node* parentNode, Node* childNode)
{
    const int parentNodeId = parentNode->getComponentID().getIntValue();
    const int childNodeId  = childNode->getComponentID().getIntValue();

    juce::ValueTree parentMidiNotesData = applicationContext.valueTreeState->getMidiNotes(parentNodeId);
    juce::ValueTree parentMidiNoteData  = parentMidiNotesData.getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    auto arrow = std::make_unique<Arrow>(parentNode, childNode, applicationContext);

    parentNode->nodeArrows[childNodeId] = arrow.get();
    canvas.addAndMakeVisible(arrow.get());

    if (parentNode->nodeType == NodeType::TraversalFlag) {
        arrow->sourceHovered = parentNode->isHovered;
        arrow->initHoverState(parentNode->isHovered);
    }

    arrow->toBack();
    arrow->setInterceptsMouseClicks(false, false);

    if (parentMidiNoteData.isValid() && childNode->nodeType != NodeType::Root) {
        arrow->bindToProperty(parentMidiNoteData, ValueTreeIdentifiers::MidiDuration);
    }

    Arrow* raw = arrow.release();
    arrows.add(raw);
    return raw;
}

void ArrowManager::adopt(Arrow* arrow)
{
    if (arrow != nullptr) {
        arrows.add(arrow);
    }
}

void ArrowManager::remove(Arrow* arrow)
{
    if (arrow == nullptr) {
        return;
    }

    if (arrow->startNode != nullptr && arrow->endNode != nullptr) {
        const int childNodeId = arrow->endNode->getComponentID().getIntValue();
        arrow->startNode->nodeArrows.erase(childNodeId);
    }

    const int index = arrows.indexOf(arrow);
    if (index >= 0) {
        canvas.removeChildComponent(arrow);
        arrows.remove(index);
    }
}

void ArrowManager::removeForNode(Node* node)
{
    for (int i = arrows.size() - 1; i >= 0; i--) {
        Arrow* arrow = arrows[i];

        if (arrow->isDangling()) {
            continue;
        }
        if (arrow->startNode != node && arrow->endNode != node) {
            continue;
        }

        const int childNodeId = arrow->endNode->getComponentID().getIntValue();
        arrow->startNode->nodeArrows.erase(childNodeId);

        arrows.remove(i);
    }
}

void ArrowManager::removeMatching(const std::function<bool(Arrow*)>& predicate)
{
    for (int i = arrows.size() - 1; i >= 0; --i) {
        Arrow* arrow = arrows[i];

        if (predicate(arrow)) {
            canvas.removeChildComponent(arrow);
            arrows.remove(i);
        }
    }
}

void ArrowManager::clear()
{
    hideSnapGhost();
    arrows.clear();
}

void ArrowManager::refreshFor(Node* movedNode)
{
    for (Arrow* arrow : arrows) {
        Node* parentNode = arrow->startNode;
        Node* childNode  = arrow->endNode;

        if (parentNode != movedNode && childNode != movedNode) {
            continue;
        }

        if (! arrow->isDangling()) {
            const NodeDisplayMode mode = movedNode->mode;
            parentNode->nodeTextEditor->formatDisplay(mode);
            childNode->nodeTextEditor->formatDisplay(mode);
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

    Node* startNode = parentNode;
    Node* endNode   = childNode;

    if (childNode->nodeValueTree.getType() == ValueTreeIdentifiers::AlternativeNodeData) {
        startNode = childNode;
        endNode   = parentNode;
    }

    const int endNodeId = endNode->getComponentID().getIntValue();
    if (startNode->nodeArrows.count(endNodeId) > 0) {
        return;
    }

    endNode->nodeColour = startNode->nodeColour;
    connect(startNode, endNode);
    refreshFor(endNode);

    juce::ValueTree rootTree = applicationContext.valueTreeState->getRootNode(parentNodeId);
    if (rootTree.isValid()) {
        applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
    }
}

void ArrowManager::handleArrowRemoved(int parentNodeId, int childNodeId)
{
    Arrow* target = find(parentNodeId, childNodeId);

    if (target == nullptr) {
        return;
    }

    remove(target);

    juce::ValueTree rootTree = applicationContext.valueTreeState->getRootNode(parentNodeId);
    if (rootTree.isValid()) {
        applicationContext.rtGraphBuilder->makeRTGraph(rootTree);
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
    for (Arrow* arrow : arrows) {
        if (arrow->selected) {
            arrow->selected = false;
            arrow->repaint();
        }
    }
}

void ArrowManager::resetAllProgress()
{
    for (Arrow* arrow : arrows) {
        if (arrow != nullptr) {
            arrow->resetProgress();
        }
    }
}

void ArrowManager::resetGraphProgress(int graphId, int traversalId)
{
    for (Arrow* arrow : arrows) {
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

void ArrowManager::triggerSnapForNode(int nodeId)
{
    Node* node = canvas.nodeManager.find(nodeId);
    if (node == nullptr) {
        return;
    }

    for (Arrow* arrow : arrows) {
        if (arrow->endNode == node) {
            arrow->triggerSnapAnimation();
            return;
        }
    }
}

void ArrowManager::showSnapGhost(Node* from, Node* to)
{
    if (snapGhostArrow != nullptr) {
        if (snapGhostArrow->startNode == from && snapGhostArrow->endNode == to) {
            return;
        }
        canvas.removeChildComponent(snapGhostArrow);
        delete snapGhostArrow;
        snapGhostArrow = nullptr;
    }

    snapGhostArrow = new Arrow(from, to, applicationContext);
    snapGhostArrow->isGhost = true;
    snapGhostArrow->setInterceptsMouseClicks(false, false);
    canvas.addAndMakeVisible(snapGhostArrow);
    snapGhostArrow->toBack();
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
