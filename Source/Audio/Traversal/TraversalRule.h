#pragma once

#include "../../Graph/RTData.h"
#include "NodeStateTable.h"

/*
    The policy for where a traversal goes next: given a node, a rule picks which of its children
    the walk moves to. TraversalLogic asks it at every step unless a script makes the choice.
    selectChild picks a child from a RuleContext, and selectDanglingArrow picks one of a node's
    dangling arrows. RuleContext is the situation a rule decides in: the graph, the parent and its
    count, the traversal and a random value.

    RuleContext::eligibleChild first rules out the children the walk may not take: arrows into
    another tree's root, tree-jump arrows unless they are allowed, arrows of zero length, children
    whose count limit is 0 or whose trigger limit is used up, the wrong kind of node for this walk,
    and arrows switched off for this traversal. Every rule chooses from the same children.

    NativeTraversalRule is the built-in choice. Of the remaining children whose count limit divides
    evenly into the parent's count, it keeps those with the highest limit and picks one at random,
    weighted by probability. If their probabilities add up to less than 100, it may pick none.
    selectDanglingArrow picks a dangling arrow by the same count-limit test.

    TraversalSession gives NativeTraversalRule::instance to its pools. TraversalLogic asks it for
    the next child when no script is making that choice, and TraversalDispatcher asks it for
    dangling arrows.
*/

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
