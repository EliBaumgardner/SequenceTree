#pragma once

#include "../Graph/RTData.h"
#include "NodeStateTable.h"

using ChildPredicate = bool (*)(RTNode::NodeType);

struct RuleContext
{
    const NodeMap&        nodes;
    const RTNode&         parent;
    int                   parentCount;
    TraversalKey          traversalKey;
    ChildPredicate        isEligible;
    const NodeStateTable& nodeState;
    int                   randomValue;

    bool allowTreeJumpChildren = false;

    const RTNode* eligibleChild(int childId) const;
};

class TraversalRule
{
public:

    virtual ~TraversalRule() = default;

    virtual int selectChild(const RuleContext& context) const = 0;

    virtual int selectDanglingArrow(const RTNode& node, int count, const TraversalKey& traversalKey) const;
};

class NativeTraversalRule : public TraversalRule
{
public:

    int selectChild(const RuleContext& context) const override;

    static const NativeTraversalRule& instance();
};
