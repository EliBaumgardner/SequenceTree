#include "RuleReorderGestures.h"
#include "../../Graph/TraversalRuleState.h"

RuleReorderGestures::RuleReorderGestures(TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager)
    : traversalRuleState(traversalRuleState),
      undoManager       (undoManager)
{
}

void RuleReorderGestures::updateDrop(std::span<const RuleTreeItem> items, int targetIndex, float fraction)
{
    const RuleTreeItem& dragged = items[static_cast<size_t>(pressedItem)];

    int targetParent = 0;

    dropItem      = targetIndex;
    dropPlacement = DropPlacement::End;

    if (targetIndex >= 0) {
        const RuleTreeItem& target = items[static_cast<size_t>(targetIndex)];

        targetParent  = target.parentId;
        dropPlacement = DropPlacement::After;

        if (fraction < halfway) {
            dropPlacement = DropPlacement::Before;
        }

        if (target.kind == RuleItemKind::Folder && fraction >= insideEdge && (target.expanded || fraction <= 1.0f - insideEdge)) {
            targetParent  = target.id;
            dropPlacement = DropPlacement::Inside;
        }
    }

    if (targetIndex == pressedItem || (dragged.kind == RuleItemKind::Folder && traversalRuleState.isInsideFolder(targetParent, dragged.id))) {
        dropPlacement = DropPlacement::None;
    }
}

void RuleReorderGestures::moveDraggedItem(std::span<const RuleTreeItem> items)
{
    RuleTreeItem dragged;

    const int itemCount = static_cast<int>(items.size());
    int       parentId  = 0;
    int       beforeId  = -1;

    if (dropPlacement == DropPlacement::None || pressedItem < 0 || pressedItem >= itemCount) {
        return;
    }

    dragged = items[static_cast<size_t>(pressedItem)];

    if (dropPlacement == DropPlacement::Inside) {
        parentId = items[static_cast<size_t>(dropItem)].id;
    }

    if (dropPlacement == DropPlacement::Before || dropPlacement == DropPlacement::After) {
        const RuleTreeItem& target = items[static_cast<size_t>(dropItem)];

        parentId = target.parentId;

        if (target.kind == dragged.kind && dropPlacement == DropPlacement::Before) {
            beforeId = target.id;
        }
        else if (target.kind == dragged.kind) {
            beforeId = nextSiblingId(items, dropItem);
        }

        for (int i = 0; target.kind != dragged.kind && dragged.kind == RuleItemKind::Rule && i < itemCount && beforeId < 0; ++i) {
            const RuleTreeItem& candidate = items[static_cast<size_t>(i)];

            if (candidate.kind == RuleItemKind::Rule && candidate.parentId == parentId && candidate.id != dragged.id) {
                beforeId = candidate.id;
            }
        }
    }

    undoManager.beginNewTransaction();

    if (dragged.kind == RuleItemKind::Rule) {
        traversalRuleState.moveRule(dragged.id, parentId, beforeId, &undoManager);
    }
    else {
        traversalRuleState.moveFolder(dragged.id, parentId, beforeId, &undoManager);
    }
}

void RuleReorderGestures::reset()
{
    dropPlacement = DropPlacement::None;
    pressedItem   = -1;
    dropItem      = -1;
    dragging      = false;
}

int RuleReorderGestures::nextSiblingId(std::span<const RuleTreeItem> items, int itemIndex) const
{
    const RuleTreeItem& item = items[static_cast<size_t>(itemIndex)];

    for (int index = itemIndex + 1; index < static_cast<int>(items.size()); ++index) {
        const RuleTreeItem& candidate = items[static_cast<size_t>(index)];

        if (candidate.depth < item.depth) {
            return -1;
        }

        if (candidate.depth == item.depth && candidate.kind == item.kind && candidate.parentId == item.parentId) {
            return candidate.id;
        }
    }

    return -1;
}
