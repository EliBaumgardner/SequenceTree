#include "OrnamentApplier.h"
#include "../UI/Canvas/NodeCanvas.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include "../UI/Node/Node.h"
#include "../Graph/OrnamentLibraries.h"
#include "../Graph/ValueTreeIdentifiers.h"

OrnamentApplier::OrnamentApplier(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager)
{
}

void OrnamentApplier::setActive(bool shouldBeActive)
{
    isActive = shouldBeActive;

    if (onActiveChanged) {
        onActiveChanged(shouldBeActive);
    }

    if (! shouldBeActive) {
        nodeCanvas.setMouseCursor(juce::MouseCursor::NormalCursor);

        return;
    }

    nodeCanvas.setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void OrnamentApplier::respondToClick(juce::Component* clickedComponent)
{
    Node* const          clickedNode = dynamic_cast<Node*>(clickedComponent);
    const juce::String   activeUid   = ornamentLibraries.libraries.getProperty(ValueTreeIdentifiers::ActiveOrnamentUid).toString();
    const OrnamentStatus status      = ornamentLibraries.statusOf(ornamentLibraries.findOrnament(activeUid));

    if (clickedNode == nullptr) {
        setActive(false);

        return;
    }

    if ((clickedNode->nodeType != NodeType::Node && clickedNode->nodeType != NodeType::Root) || status == OrnamentStatus::Draft) {
        return;
    }

    undoManager.beginNewTransaction();

    clickedNode->nodeValueTree.setProperty(ValueTreeIdentifiers::OrnamentUid, activeUid, &undoManager);
}

void OrnamentApplier::respondToMarkClick(const juce::MouseEvent& event, OrnamentMarkView& markView)
{
    juce::ValueTree ornamentedNode = markView.nodeValueTree;
    ContextMenu     menu;

    if (! event.mods.isRightButtonDown()) {
        return;
    }

    if (event.mods.isShiftDown()) {
        undoManager.beginNewTransaction();

        ornamentedNode.removeProperty(ValueTreeIdentifiers::OrnamentUid, &undoManager);

        return;
    }

    menu.addItem("delete ornament", ContextMenu::ItemKind::Action, [this, ornamentedNode]() mutable {
        undoManager.beginNewTransaction();

        ornamentedNode.removeProperty(ValueTreeIdentifiers::OrnamentUid, &undoManager);
    });

    menu.show(markView);
}
