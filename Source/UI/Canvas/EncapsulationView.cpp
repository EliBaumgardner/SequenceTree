#include "EncapsulationView.h"
#include "../../Graph/TypedProperties.h"

#include "NodeCanvas.h"
#include <ArsNovaUI/Canvas/NodeManager.h>
#include <ArsNovaUI/Canvas/ArrowManager.h>
#include "../Node/Node.h"
#include "../Node/Encapsulator.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <unordered_set>

EncapsulationView::EncapsulationView(NodeCanvas& nodeCanvas, GraphState& graphState)
    : nodeCanvas(nodeCanvas),
      graphState(graphState)
{
}

void EncapsulationView::collapse(int encapsulatorId) const
{
    if (! applyCollapsedState(encapsulatorId)) {
        return;
    }

    repositionAll();

    nodeCanvas.arrowManager.refreshCollapsedArrows();
}

void EncapsulationView::expand(int encapsulatorId) const
{
    auto* const encapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(encapsulatorId));

    if (encapsulator == nullptr || encapsulator->memberNodeIds.empty()) {
        return;
    }

    encapsulator->isExpanded = true;

    encapsulator->setVisible(false);
    encapsulator->setInterceptsMouseClicks(false, false);

    showMembers(encapsulator->memberNodeIds);

    refreshMembership(encapsulatorId);
}

void EncapsulationView::refreshMembership(int encapsulatorId) const
{
    auto* const encapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(encapsulatorId));

    if (encapsulator == nullptr) {
        return;
    }

    encapsulator->bindToTree();

    if (! encapsulator->isExpanded) {
        return;
    }

    for (const int memberNodeId : encapsulator->memberNodeIds) {
        Node* const member = static_cast<Node*>(nodeCanvas.nodeManager.find(memberNodeId));

        if (member == nullptr) {
            continue;
        }

        member->isEncapsulationRinged   = true;
        member->hasInnerRim             = memberNodeId == encapsulator->memberNodeIds.front();
        member->encapsulationRingColour = encapsulator->nodeColour;

        member->repaint();
    }
}

void EncapsulationView::collapseAll() const
{
    const juce::ValueTree nodeMap = graphState.nodeMap;

    for (int i = 0; i < nodeMap.getNumChildren(); ++i) {
        const juce::ValueTree nodeValueTree = nodeMap.getChild(i);

        if (nodeValueTree.getType() == ValueTreeIdentifiers::EncapsulatorData) {
            applyCollapsedState(nodeValueTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id));
        }
    }

    repositionAll();

    nodeCanvas.arrowManager.refreshCollapsedArrows();
}

void EncapsulationView::showMembers(std::span<const int> memberNodeIds) const
{
    for (const int memberNodeId : memberNodeIds) {
        Node* const member = static_cast<Node*>(nodeCanvas.nodeManager.find(memberNodeId));

        if (member == nullptr) {
            continue;
        }

        member->isCollapsed        = false;
        member->isCollapsedExit   = false;
        member->isEncapsulationRinged = false;
        member->hasInnerRim           = false;

        member->setVisible(true);
        member->setInterceptsMouseClicks(nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint,
                                         nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint && nodeCanvas.activeTool != NodeCanvas::CanvasTool::Span);
    }

    repositionAll();

    nodeCanvas.arrowManager.refreshCollapsedArrows();
}

void EncapsulationView::syncHighlights() const
{
    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (auto* const encapsulator = dynamic_cast<Encapsulator*>(node.get())) {
            encapsulator->syncHighlightsFromMembers();
        }
    }
}

void EncapsulationView::recolourGroup(const Encapsulator& encapsulator) const
{
    for (const int memberNodeId : encapsulator.memberNodeIds) {
        Node* const member = static_cast<Node*>(nodeCanvas.nodeManager.find(memberNodeId));

        if (member == nullptr) {
            continue;
        }

        member->encapsulationRingColour = encapsulator.nodeColour;

        member->repaint();
    }
}

juce::Point<int> EncapsulationView::collapsedSpanShift(int nodeId) const
{
    const juce::ValueTree node = graphState.getNode(nodeId);

    const NodeProperties properties(node);

    int walkStartId       = nodeId;
    int ownEncapsulatorId = properties.encapsulatorId;

    if (node.getType() == ValueTreeIdentifiers::EncapsulatorData) {
        const std::vector<int> memberNodeIds = graphState.encapsulation.memberIds(nodeId);

        if (memberNodeIds.empty()) {
            return {};
        }

        walkStartId       = memberNodeIds.front();
        ownEncapsulatorId = nodeId;
    }

    juce::Point<int> shift;

    std::unordered_set<int> visited { walkStartId };
    std::unordered_set<int> shiftedEncapsulatorIds;
    std::vector<int>        frontier { walkStartId };

    while (! frontier.empty()) {
        std::vector<int> nextFrontier;

        for (const int currentId : frontier) {
            const auto parents = graphState.parentIdsOf.find(currentId);

            if (parents == graphState.parentIdsOf.end()) {
                continue;
            }

            for (const int parentId : parents->second) {
                if (! visited.insert(parentId).second) {
                    continue;
                }

                nextFrontier.push_back(parentId);

                const NodeProperties parent(graphState.getNode(parentId));

                const int encapsulatorId = parent.encapsulatorId;

                if (encapsulatorId < 0 || encapsulatorId == ownEncapsulatorId) {
                    continue;
                }

                const juce::ValueTree encapsulator = graphState.getNode(encapsulatorId);

                if (! encapsulator.isValid() || ! shiftedEncapsulatorIds.insert(encapsulatorId).second) {
                    continue;
                }

                auto* const encapsulatorNode =
                    dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(encapsulatorId));

                if (encapsulatorNode != nullptr && encapsulatorNode->isExpanded) {
                    continue;
                }

                const NodeInfo::Position exitPosition         = graphState.getNodePosition(parentId);
                const NodeInfo::Position encapsulatorPosition = graphState.getNodePosition(encapsulatorId);

                shift.x -= exitPosition.xPosition - encapsulatorPosition.xPosition;
                shift.y -= exitPosition.yPosition - encapsulatorPosition.yPosition;
            }
        }

        frontier = std::move(nextFrontier);
    }

    return shift;
}

bool EncapsulationView::applyCollapsedState(int encapsulatorId) const
{
    auto* const encapsulator = dynamic_cast<Encapsulator*>(nodeCanvas.nodeManager.find(encapsulatorId));

    if (encapsulator == nullptr) {
        return false;
    }

    encapsulator->isExpanded = false;

    encapsulator->bindToTree();

    encapsulator->setVisible(true);
    encapsulator->setInterceptsMouseClicks(nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint,
                                           nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint && nodeCanvas.activeTool != NodeCanvas::CanvasTool::Span);

    for (const int memberNodeId : encapsulator->memberNodeIds) {
        Node* const member = static_cast<Node*>(nodeCanvas.nodeManager.find(memberNodeId));

        if (member == nullptr) {
            continue;
        }

        member->isCollapsed        = true;
        member->isCollapsedExit   = memberNodeId == encapsulator->memberNodeIds.back();
        member->isOutlined            = false;
        member->isEncapsulationRinged = false;
        member->hasInnerRim           = false;

        member->setVisible(false);
        member->setInterceptsMouseClicks(false, false);
    }

    encapsulator->syncHighlightsFromMembers();
    encapsulator->toFront(false);

    return true;
}

void EncapsulationView::repositionAll() const
{
    for (const auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        nodeCanvas.nodeManager.setPosition(nodeId);
    }
}
