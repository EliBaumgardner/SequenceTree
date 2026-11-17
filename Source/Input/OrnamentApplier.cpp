#include "OrnamentApplier.h"
#include "../Graph/TypedProperties.h"
#include "../UI/Canvas/NodeCanvas.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include "../UI/Node/Node.h"
#include "../Graph/OrnamentLibraries.h"
#include "../Graph/OrnamentFigure.h"
#include "../Graph/ValueTreeIdentifiers.h"

OrnamentApplier::OrnamentApplier(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : nodeCanvas       (nodeCanvas),
      ornamentLibraries(ornamentLibraries),
      undoManager      (undoManager)
{
}

void OrnamentApplier::respondToClick(juce::Component* clickedComponent)
{
    Node* const clickedNode = dynamic_cast<Node*>(clickedComponent);

    const juce::String activeUid = ornamentLibraries.activeOrnamentUid;

    const OrnamentStatus status = OrnamentFigure::statusOf(ornamentLibraries.findOrnament(activeUid));

    if (clickedNode == nullptr) {
        nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);

        return;
    }

    if ((clickedNode->nodeType != NodeInfo::Type::Node && clickedNode->nodeType != NodeInfo::Type::Root) || status == OrnamentStatus::Draft) {
        return;
    }

    NodeProperties properties(clickedNode->nodeValueTree);

    undoManager.beginNewTransaction();

    properties.ornamentUid.setValue(activeUid, &undoManager);
}

void OrnamentApplier::respondToMarkClick(const juce::MouseEvent& event, OrnamentMarkView& markView)
{
    juce::ValueTree        ornamentedNode = markView.nodeValueTree;
    ArsNovaUI::ContextMenu menu;

    if (! event.mods.isRightButtonDown()) {
        return;
    }

    if (event.mods.isShiftDown()) {
        NodeProperties properties(ornamentedNode);

        undoManager.beginNewTransaction();

        properties.ornamentUid.resetToDefault(&undoManager);

        return;
    }

    menu.addItem("delete ornament", ArsNovaUI::ContextMenu::ItemKind::Action, [this, ornamentedNode]() mutable {
        NodeProperties properties(ornamentedNode);

        undoManager.beginNewTransaction();

        properties.ornamentUid.resetToDefault(&undoManager);
    });

    menu.show(markView);
}
