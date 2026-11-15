#pragma once

#include "ChildSelector.h"

#include <optional>

class ModulatorWalk
{
public:

    enum class Change;

    void activate(int rootId, int hostId);
    void deactivate ();
    void decide  (const NodeMap& nodes, ChildSelector& selector, TraversalMode mode, std::optional<int> scriptChoice);

    Change followMainWalk(const NodeMap& nodes, ChildSelector& selector, const RTNode& enteredNode, bool isPrimaryRepeat);

    int findActiveModulatorRoot(const NodeMap& nodes, const ChildSelector& selector, int regularNodeId) const;

    bool isActive () const;
    bool step ();
    bool tickRepeat(int repeatLimit);

    struct ModulatorGate
    {
        int activeRootId = -1;
        int hostId       = -1;
        int repeatCount  = 0;
    };

    enum class Change { None, Started, Stopped, Advanced, Restarted };

    Walker        walker;
    ModulatorGate gate;

    int decidedTarget = -1;

    bool decidedRestart = false;

private:

    const RTNode* eligibleModulatorRoot(const NodeMap& nodes, const TraversalKey& traversalKey, const RTConnection& connection, int hostCount) const;

    static bool isDescendantOf(const NodeMap& nodes, int nodeId, int ancestorId);

    static constexpr int maximumAncestorSteps = 10000;
};
