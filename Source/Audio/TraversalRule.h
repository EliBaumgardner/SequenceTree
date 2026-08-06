#pragma once

#include "../Graph/RTData.h"
#include "NodeStateTable.h"

using ChildPredicate = bool (*)(RTNode::NodeType);

struct RuleContext
{
    const NodeMap&        nodes;
    const RTNode&         parent;
    int                   parentCount;
    int                   traversalId;
    ChildPredicate        isEligible;
    const NodeStateTable& nodeState;

    bool allowTreeJumpChildren = false;

    const RTNode* eligibleChild(int childId) const;
};

class TraversalRule
{
public:

    virtual ~TraversalRule() = default;

    virtual int selectChild(const RuleContext& context) const = 0;
};

class NativeTraversalRule : public TraversalRule
{
public:

    int selectChild(const RuleContext& context) const override;

    static const NativeTraversalRule& instance();
};
