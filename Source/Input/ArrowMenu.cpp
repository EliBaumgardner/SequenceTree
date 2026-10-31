#include "ArrowMenu.h"
#include "GraphDocument.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Node/Node.h"
#include "../UI/Node/Arrow.h"
#include "../UI/Menus/AllowedTraversalsMenu.h"
#include "../Graph/GraphState.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

ArrowMenu::ArrowMenu(NodeCanvas& nodeCanvas, GraphState& graphState, GraphDocument& graphDocument, juce::UndoManager& undoManager)
    : nodeCanvas(nodeCanvas),
      graphState(graphState),
      graphDocument(graphDocument),
      undoManager(undoManager)
{
}

void ArrowMenu::showArrowContextMenu(Arrow* arrow)
{
    ArsNovaUI::ContextMenu menu;

    if (arrow == nullptr) {
        return;
    }

    menu.addItem("edit allowed traversals", ArsNovaUI::ContextMenu::ItemKind::Action, [this, arrow]() {
        juce::ValueTree connection = connectionTreeFor(arrow);

        if (! connection.isValid()) {
            return;
        }

        allowedTraversalsLauncher.show(nodeCanvas, [this, connection]() {
            auto content = std::make_unique<AllowedTraversalsMenu>(ArsNovaUI::CustomLookAndFeel::get(nodeCanvas), graphState, undoManager, connection);

            content->setSize(AllowedTraversalsMenu::defaultWidth, content->getIdealHeight());

            return content;
        });
    });

    menu.addItem("traversal arrow", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this, arrow]() {
        if (arrow->isTraversalArrow()) {
            setArrowType(arrow, ArrowInfo::Type::Node);
            return;
        }

        setArrowType(arrow, ArrowInfo::Type::Traversal);
    }, canBeTraversalArrow(arrow), arrow->isTraversalArrow());

    if (connectsToModulatorRoot(arrow)) {
        menu.addItem("sync", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this, arrow]() {
            setArrowSync(arrow, ! arrow->isSyncArrow());
        }, true, arrow->isSyncArrow());
    }

    menu.show(*arrow);
}

void ArrowMenu::setArrowType(const Arrow* arrow, ArrowInfo::Type arrowType)
{
    if (! canBeTraversalArrow(arrow)) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = graphDocument.ownershipOf(arrow->startNode->nodeId, arrow->endNode->nodeId);

    const juce::ValueTree connection = graphState.getConnection(ownerNodeId, childNodeId);

    ArrowInfo::Settings arrowInfo = ArrowBindingOps::getArrowInfo(connection);
    arrowInfo.type                = arrowType;

    undoManager.beginNewTransaction();
    ArrowBindingOps::setArrowInfo(connection, arrowInfo, &undoManager);
}

void ArrowMenu::setArrowSync(const Arrow* arrow, bool shouldSync)
{
    if (! connectsToModulatorRoot(arrow)) {
        return;
    }

    const auto [ownerNodeId, childNodeId] = graphDocument.ownershipOf(arrow->startNode->nodeId, arrow->endNode->nodeId);

    const juce::ValueTree connection = graphState.getConnection(ownerNodeId, childNodeId);

    ArrowInfo::Settings arrowInfo = ArrowBindingOps::getArrowInfo(connection);
    arrowInfo.isSynced            = shouldSync;

    undoManager.beginNewTransaction();
    ArrowBindingOps::setArrowInfo(connection, arrowInfo, &undoManager);
}

bool ArrowMenu::canBeTraversalArrow(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = graphDocument.ownershipOf(arrow->startNode->nodeId, arrow->endNode->nodeId);

    return graphState.connectsToOtherTreeRoot(ownerNodeId, childNodeId);
}

bool ArrowMenu::connectsToModulatorRoot(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr || arrow->endNode == nullptr) {
        return false;
    }

    const auto [ownerNodeId, childNodeId] = graphDocument.ownershipOf(arrow->startNode->nodeId, arrow->endNode->nodeId);

    const juce::ValueTree childTree = graphState.getNode(childNodeId);

    return childTree.getType() == ValueTreeIdentifiers::ModulatorRootData;
}

juce::ValueTree ArrowMenu::connectionTreeFor(const Arrow* arrow) const
{
    if (arrow == nullptr || arrow->startNode == nullptr) {
        return {};
    }

    if (arrow->isDangling()) {
        return arrow->arrowTree;
    }

    const auto [ownerNodeId, childNodeId] = graphDocument.ownershipOf(arrow->startNode->nodeId, arrow->endNode->nodeId);

    juce::ValueTree ownerTree     = graphState.getNode(ownerNodeId);
    juce::ValueTree ownerChildren = ownerTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    return ownerChildren.getChildWithProperty(ValueTreeIdentifiers::Id, childNodeId);
}
