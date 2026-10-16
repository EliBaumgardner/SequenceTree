#include "SelectionGestures.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../Graph/GraphState.h"
#include <ArsNovaUI/Menus/ContextMenu.h>

void SelectionGestures::copySelection()
{
    const std::vector<int> ids = selectedNodeIds();

    if (ids.empty()) {
        return;
    }

    clipboard.copy(ids);
}

void SelectionGestures::deleteSelection()
{
    const std::vector<int> ids = selectedNodeIds();
    if (ids.empty()) {
        return;
    }

    undoManager.beginNewTransaction();

    for (const int nodeId : ids) {
        const juce::ValueTree node = graphState.getNode(nodeId);
        if (!node.isValid()) {
            continue;
        }

        graphState.removeNode(nodeId, &undoManager);
    }
}

void SelectionGestures::pasteAt(juce::Point<int> canvasPoint)
{
    if (clipboard.contents.getNumChildren() == 0) {
        return;
    }

    const std::vector<int> pastedIds = clipboard.pasteAt(canvasPoint);

    nodeCanvas.handleAsyncUpdate();

    clearAll();

    for (const int nodeId : pastedIds) {
        Node* const node = nodeCanvas.nodeManager.find(nodeId);

        if (node != nullptr && node->isVisible()) {
            node->setSelectVisual(true);
        }
    }
}

void SelectionGestures::beginBoxSelection(const juce::Point<int>& clickPoint)
{
    selectionAnchor = clickPoint;

    nodeCanvas.selectionBounds = {};

    clearAll();
}

void SelectionGestures::updateBoxSelection(const juce::MouseEvent& e)
{
    const juce::Point<int> cursor = e.getEventRelativeTo(&nodeCanvas).position.transformedBy(nodeCanvas.modelTransform).roundToInt();

    nodeCanvas.selectionBounds = juce::Rectangle<int>(selectionAnchor, cursor);

    nodeCanvas.repaint();
}

void SelectionGestures::finishBoxSelection()
{
    const juce::Rectangle<int> selection = nodeCanvas.selectionBounds;

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node->isVisible() && selection.contains(node->getNodeCentre())) {
            node->setSelectVisual(true);
        }
    }

    nodeCanvas.selectionBounds = {};

    nodeCanvas.repaint();
}

void SelectionGestures::showSelectionMenu(juce::Point<int> canvasPoint)
{
    const bool  hasCopiedNodes = ! selectedNodeIds().empty();
    const bool  hasClipboard   = clipboard.contents.getNumChildren() > 0;
    ContextMenu menu;

    menu.addItem("copy",   ContextMenu::ItemKind::Action, [this]() { copySelection(); }, hasCopiedNodes);
    menu.addItem("paste",  ContextMenu::ItemKind::Action, [this, canvasPoint]() { pasteAt(canvasPoint); }, hasClipboard);
    menu.addItem("delete", ContextMenu::ItemKind::Action, [this]() { deleteSelection(); }, hasCopiedNodes);

    menu.show(nodeCanvas);
}

void SelectionGestures::selectAll() const
{
    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node->isVisible() && ! node->isSelected) {
            node->setSelectVisual(true);
        }
    }
}

void SelectionGestures::clearAll() const
{
    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node->isSelected) {
            node->setSelectVisual(false);
        }
    }
}

void SelectionGestures::deselectAllExcept(const Node& keptNode) const
{
    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node.get() != &keptNode && node->isSelected) {
            node->setSelectVisual(false);
        }
    }
}

std::vector<int> SelectionGestures::selectedNodeIds() const
{
    std::vector<int> ids;

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node->isSelected) {
            ids.push_back(nodeId);
        }
    }

    return ids;
}
