 #include "TraversalRule.h"

#include <algorithm>

const RTNode* RuleContext::eligibleChild(int childId) const
{
    const RTConnection* const connection = parent.findConnection(childId);

    const bool isTreeJumpChild = connection != nullptr && connection->isTreeJump;

    if (isTreeJumpChild && !allowTreeJumpChildren) {
        return nullptr;
    }

    const auto childIt = nodes.find(childId);
    if (childIt == nodes.end()) {
        return nullptr;
    }

    const RTNode& child = *childIt->second;

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
        const std::vector<int>& disabled = connection->disabledTraversals;
        if (std::find(disabled.begin(), disabled.end(), traversalId) != disabled.end()) {
            return nullptr;
        }
    }

    return &child;
}

int TraversalRule::selectDanglingArrow(const RTNode& node, int count, int traversalId) const
{
    int chosen   = -1;
    int maxLimit = 0;

    for (std::size_t index = 0; index < node.danglingArrows.size(); ++index) {
        const RTNode::DanglingArrow& dangling = node.danglingArrows[index];

        if (dangling.duration <= 0 || dangling.countLimit <= 0) {
            continue;
        }

        const std::vector<int>& disabled = dangling.disabledTraversals;
        if (std::find(disabled.begin(), disabled.end(), traversalId) != disabled.end()) {
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
    int maxLimit    = 0;
    int totalWeight = 0;

    for (const RTConnection& connection : context.parent.connections) {
        const RTNode* child = context.eligibleChild(connection.childId);

        if (child == nullptr) {
            continue;
        }

        if (context.parentCount % child->countLimit != 0) {
            continue;
        }

        if (child->countLimit > maxLimit) {
            maxLimit    = child->countLimit;
            totalWeight = child->probability;
        }
        else if (child->countLimit == maxLimit) {
            totalWeight += child->probability;
        }
    }

    if (totalWeight <= 0) {
        return -1;
    }

    int selectionSpan = totalWeight;

    if (selectionSpan < RTNode::probabilityScale) {
        selectionSpan = RTNode::probabilityScale;
    }

    const int pick = context.randomValue % selectionSpan;

    int runningWeight = 0;

    for (const RTConnection& connection : context.parent.connections) {
        const RTNode* child = context.eligibleChild(connection.childId);

        if (child == nullptr) {
            continue;
        }

        if (context.parentCount % child->countLimit != 0 || child->countLimit != maxLimit) {
            continue;
        }

        runningWeight += child->probability;

        if (pick < runningWeight) {
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
