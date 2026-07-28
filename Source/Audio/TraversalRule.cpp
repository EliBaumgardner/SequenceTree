#include "TraversalRule.h"

const RTNode* RuleContext::eligibleChild(int childId) const
{
    const auto childIt = nodes.find(childId);
    if (childIt == nodes.end()) {
        return nullptr;
    }

    const RTNode& child = childIt->second;

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

    const auto durIt = parent.durationMap.find(childId);
    if (durIt != parent.durationMap.end() && durIt->second == 0) {
        return nullptr;
    }

    const auto disabledIt = parent.disabledTraversalsByChild.find(childId);
    if (disabledIt != parent.disabledTraversalsByChild.end()
        && disabledIt->second.count(traversalId) > 0) {
        return nullptr;
    }

    return &child;
}

int NativeTraversalRule::selectChild(const RuleContext& context) const
{
    int chosen   = -1;
    int maxLimit = 0;

    for (const int childId : context.parent.children) {
        const RTNode* child = context.eligibleChild(childId);

        if (child == nullptr) {
            continue;
        }

        if (context.parentCount % child->countLimit == 0 && child->countLimit > maxLimit) {
            chosen   = childId;
            maxLimit = child->countLimit;
        }
    }

    return chosen;
}

const NativeTraversalRule& NativeTraversalRule::instance()
{
    static const NativeTraversalRule nativeRule;
    return nativeRule;
}
