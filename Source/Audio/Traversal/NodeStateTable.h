#pragma once

#include "NodeRowMap.h"

#include <cstddef>
#include <vector>

/*
    Everything one traversal remembers about the nodes it has passed through: for each node a whole
    number per NodeStateSlot, saying how many times the walk has left the node, its switch,
    sub-loop, modulator and trigger counts, which alternative is active and which child was chosen
    last. Those counts decide which children the traversal may take next, so each traversal keeps
    its own table.

    get, set, increment and ref read and write one value by slot and node id. A NodeRowMap gives
    each node id its row the first time it is written, and the values sit in one fixed block,
    a row per node and a column per slot. A node that was never written reads as its default: -1
    for slots that hold a node id, 0 for counts. It can hold 1024 different nodes. Writes past that
    are thrown away, and assert in debug builds. clear puts every value back to its default.

    Each TraversalLogic owns one, so every traversal keeps its own counts, and clears it when the
    traversal restarts. TraversalPool::prepare reserves its memory before playback, so
    reading and writing it never allocates. The traversal rule and scripts read it to decide which
    children can be chosen, and scripts can change it.
*/

enum class NodeStateSlot
{
    Count,
    SwitchCount,
    SubRootCount,
    ModulatorCount,
    Trigger,
    Chord,
    CrossTree,
    CrossTreeSwitch,
    ActiveAlternative,
    LastNode,
    SwitchCandidate,
    Total
};

class NodeStateTable
{
public:

    static constexpr int         slotCount  = static_cast<int>(NodeStateSlot::Total);
    static constexpr int         maxNodeIds = NodeRowMap::maxRows;
    static constexpr std::size_t valueCount = static_cast<std::size_t>(slotCount) * maxNodeIds;

    void prepare();
    void clear  ();

    int  increment(NodeStateSlot slot, int nodeId);
    int  get      (NodeStateSlot slot, int nodeId) const;
    int& ref      (NodeStateSlot slot, int nodeId);
    void set      (NodeStateSlot slot, int nodeId, int value);

    static int defaultValue(NodeStateSlot slot);

private:

    bool isAddressable(int nodeId) const;

    static int indexOf(NodeStateSlot slot, int row);

    std::vector<int> values;
    NodeRowMap       rows;

    int outOfRangeSink = 0;
};
