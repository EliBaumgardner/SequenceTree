//
// Created by Eli Baumgardner on 7/21/26.
//

#include "DanglingArrowLayer.h"

#include "NodeCanvas.h"
#include "../Node/Arrow.h"
#include "../Node/Node.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/ApplicationContext.h"

DanglingArrowLayer::DanglingArrowLayer(NodeCanvas& canvasRef, ApplicationContext& context)
    : canvas(canvasRef), applicationContext(context)
{
}

DanglingArrowLayer::~DanglingArrowLayer() = default;

void DanglingArrowLayer::setArrowMode(bool enabled)
{
    arrowMode = enabled;

    if (!enabled) {
        cancelPreview();
    }
}

void DanglingArrowLayer::updatePreview(Node* node, juce::Point<int> tipOffset, bool dashed)
{
    if (node == nullptr) {
        return;
    }

    if (preview == nullptr || preview->startNode != node) {
        preview = std::make_unique<Arrow>(node, tipOffset, applicationContext);
        canvas.addAndMakeVisible(*preview);
        preview->toBack();
    }

    preview->dashed = dashed;
    preview->setTipOffset(tipOffset);
}

void DanglingArrowLayer::commitPreview()
{
    if (preview == nullptr) {
        return;
    }

    Node* node = preview->startNode;
    juce::Point<int> tipOffset = preview->tipOffset;

    preview.reset();

    add(node, tipOffset);
}

void DanglingArrowLayer::cancelPreview()
{
    preview.reset();
}

void DanglingArrowLayer::add(Node* node, juce::Point<int> tipOffset)
{
    if (node == nullptr || !node->nodeValueTree.isValid()) {
        return;
    }

    juce::UndoManager* undoManager = applicationContext.undoManager;
    juce::ValueTree nodeTree = node->nodeValueTree;

    juce::ValueTree arrowList = nodeTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);
    if (!arrowList.isValid()) {
        arrowList = juce::ValueTree(ValueTreeIdentifiers::DanglingArrows);
        nodeTree.addChild(arrowList, -1, undoManager);
    }

    juce::ValueTree arrowTree(ValueTreeIdentifiers::DanglingArrow);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipX, tipOffset.x, undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipY, tipOffset.y, undoManager);
    arrowList.addChild(arrowTree, -1, undoManager);
}

void DanglingArrowLayer::remove(Arrow* arrow)
{
    if (arrow == nullptr || !arrow->arrowTree.isValid()) {
        return;
    }

    juce::ValueTree arrowList = arrow->arrowTree.getParent();
    if (arrowList.isValid()) {
        arrowList.removeChild(arrow->arrowTree, applicationContext.undoManager);
    }
}

Arrow* DanglingArrowLayer::hitTestHead(juce::Point<int> canvasPos, float radius) const
{
    Arrow* nearest = nullptr;
    float minDist = radius;

    for (Arrow* arrow : canvas.arrowManager.all()) {
        if (! arrow->isDangling() || arrow->startNode == nullptr) {
            continue;
        }

        float dist = (float) canvasPos.getDistanceFrom(arrow->getTip());
        if (dist < minDist) {
            minDist = dist;
            nearest = arrow;
        }
    }

    return nearest;
}

void DanglingArrowLayer::setTip(Arrow* arrow, juce::Point<int> tipOffset)
{
    if (arrow == nullptr) {
        return;
    }

    arrow->setTipOffset(tipOffset);
}

void DanglingArrowLayer::commitTip(Arrow* arrow)
{
    if (arrow == nullptr || !arrow->arrowTree.isValid()) {
        return;
    }

    juce::UndoManager* undoManager = applicationContext.undoManager;
    arrow->arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipX, arrow->tipOffset.x, undoManager);
    arrow->arrowTree.setProperty(ValueTreeIdentifiers::ArrowTipY, arrow->tipOffset.y, undoManager);
}

void DanglingArrowLayer::rebuildForNode(int nodeId)
{
    removeForNodeId(nodeId);

    Node* node = canvas.nodeManager.find(nodeId);
    if (node == nullptr) {
        return;
    }
    juce::ValueTree arrowList = node->nodeValueTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows);
    if (!arrowList.isValid()) {
        return;
    }

    for (int i = 0; i < arrowList.getNumChildren(); ++i) {
        juce::ValueTree arrowTree = arrowList.getChild(i);

        juce::Point<int> tipOffset {
            (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX),
            (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY)
        };

        auto arrow = std::make_unique<Arrow>(node, tipOffset, applicationContext);
        arrow->arrowTree = arrowTree;
        canvas.addAndMakeVisible(*arrow);
        arrow->toBack();
        arrow->setArrowBounds();
        canvas.arrowManager.adopt(arrow.release());
    }
}

void DanglingArrowLayer::removeForNode(Node* node)
{
    canvas.arrowManager.removeMatching([node](Arrow* arrow) {
        return arrow->isDangling() && arrow->startNode == node;
    });
}

void DanglingArrowLayer::removeForNodeId(int nodeId)
{
    canvas.arrowManager.removeMatching([nodeId](Arrow* arrow) {
        if (! arrow->isDangling() || arrow->startNode == nullptr) {
            return false;
        }
        return arrow->startNode->getComponentID().getIntValue() == nodeId;
    });
}

void DanglingArrowLayer::clear()
{
    cancelPreview();
}
