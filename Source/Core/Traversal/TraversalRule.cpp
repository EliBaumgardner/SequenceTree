#include "TraversalRule.h"
#include "WeightedPick.h"

#include <algorithm>

const RTNode* RuleContext::eligibleChild(int childId) const
{
    const RTConnection* const connection = parent.findConnection(childId);

    const bool isTreeJumpChild = connection != nullptr && connection->isTreeJump;

    if (isTreeJumpChild && !allowTreeJumpChildren) {
        return nullptr;
    }

    if (connection != nullptr && connection->isCrossRoot) {
        return nullptr;
    }

    const RTNode* const childNode = nodes.find(childId);

    if (childNode == nullptr) {
        return nullptr;
    }

    const RTNode& child = *childNode;

    if (!isEligible(child.nodeType)) {
        return nullptr;
    }

    if (child.countLimit <= 0) {
        return nullptr;
    }

    if (child.triggerLimit > 0) {
        if (nodeState.get(NodeStateSlot::Trigger, childId) >= child.triggerLimit) {
            return nullptr;
        }
    }

    if (!isTreeJumpChild && connection != nullptr && connection->duration == 0) {
        return nullptr;
    }

    if (connection != nullptr) {
        const std::vector<TraversalKey>& disabled = connection->disabledTraversals;

        if (std::ranges::find(disabled, traversalKey) != disabled.end()) {
            return nullptr;
        }
    }

    return &child;
}

int TraversalRule::selectDanglingArrow(const RTNode& node, int count, const TraversalKey& traversalKey) const
{
    int chosen   = -1;
    int maxLimit = 0;

    for (std::size_t index = 0; index < node.danglingArrows.size(); ++index) {
        const RTNode::DanglingArrow& dangling = node.danglingArrows[index];

        if (dangling.duration <= 0 || dangling.countLimit <= 0) {
            continue;
        }

        const std::vector<TraversalKey>& disabled = dangling.disabledTraversals;

        if (std::ranges::find(disabled, traversalKey) != disabled.end()) {
            continue;
        }

        if (count % dangling.countLimit == 0 && dangling.countLimit > maxLimit) {
            chosen   = static_cast<int>(index);
            maxLimit = dangling.countLimit;
        }
    }

    return chosen;
}

int NativeTraversalRule::selectChild(const RuleContext& context) const
{
    WeightedPick weightedPick;

    for (const RTConnection& connection : context.parent.connections) {
        const RTNode* child = context.eligibleChild(connection.childId);

        if (child == nullptr) {
            continue;
        }

        if (context.parentCount % child->countLimit != 0) {
            continue;
        }

        weightedPick.tally(child->countLimit, child->probability);
    }

    if (! weightedPick.draw(context.randomValue)) {
        return -1;
    }

    for (const RTConnection& connection : context.parent.connections) {
        const RTNode* child = context.eligibleChild(connection.childId);

        if (child == nullptr) {
            continue;
        }

        if (context.parentCount % child->countLimit != 0) {
            continue;
        }

        if (weightedPick.accepts(child->countLimit, child->probability)) {
            return connection.childId;
        }
    }

    return -1;
}

const NativeTraversalRule& NativeTraversalRule::instance()
{
    static const NativeTraversalRule nativeRule;

    return nativeRule;
}
