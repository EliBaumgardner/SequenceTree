#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <span>

class TraversalRuleState;

enum class RuleItemKind { Folder, Rule };

struct RuleTreeItem
{
    RuleItemKind kind = RuleItemKind::Rule;

    int id       = -1;
    int parentId = 0;
    int depth    = 0;

    bool expanded = false;
};

class RuleReorderGestures
{
public:

    RuleReorderGestures(TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager);

    void updateDrop     (std::span<const RuleTreeItem> items, int targetIndex, float fraction);
    void moveDraggedItem(std::span<const RuleTreeItem> items);
    void reset ();

    enum class DropPlacement { None, Before, After, Inside, End };

    static constexpr int dragThreshold = 4;

    static constexpr float insideEdge = 0.25f;
    static constexpr float halfway    = 0.5f;

    DropPlacement dropPlacement = DropPlacement::None;

    int pressedItem = -1;
    int dropItem    = -1;

    bool dragging = false;

private:

    int nextSiblingId(std::span<const RuleTreeItem> items, int itemIndex) const;

    TraversalRuleState& traversalRuleState;
    juce::UndoManager&  undoManager;
};
