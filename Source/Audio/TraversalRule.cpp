 #include "TraversalRule.h"

const RTNode* RuleContext::eligibleChild(int childId) const
{
    const RTNodeData* const data = findNodeData(parent, childId);

    const bool isTreeJumpChild = data != nullptr && data->isTreeJump;

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

    if (!isTreeJumpChild && data != nullptr && data->duration == 0) {
        return nullptr;
    }

    if (data != nullptr && isTraversalDisabled(data->disabledTraversals, traversalId)) {
        return nullptr;
    }

    return &child;
}

int NativeTraversalRule::selectChild(const RuleContext& context) const
{
    int chosen   = -1;
    int maxLimit = 0;

    for (const RTNodeData& data : context.parent.nodeData) {
        const int childId = data.childId;

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
