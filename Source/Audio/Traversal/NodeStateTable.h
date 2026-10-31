#pragma once

#include "NodeRowMap.h"

#include <array>
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

[[nodiscard]] constexpr int defaultNodeStateSlotValue(NodeStateSlot slot) noexcept
{
    switch (slot) {
        case NodeStateSlot::ActiveAlternative:
        case NodeStateSlot::LastNode:
        case NodeStateSlot::SwitchCandidate:
            return -1;

        default:
            return 0;
    }
}

class NodeStateTable
{
public:

    static constexpr int         slotCount  = static_cast<int>(NodeStateSlot::Total);
    static constexpr int         maxNodeIds = NodeRowMap::maxRows;
    static constexpr std::size_t valueCount = static_cast<std::size_t>(slotCount) * maxNodeIds;

    void prepare();
    void clear  () noexcept;

    int                increment(NodeStateSlot slot, int nodeId) noexcept;
    [[nodiscard]] int  get      (NodeStateSlot slot, int nodeId) const noexcept;
    [[nodiscard]] int& ref      (NodeStateSlot slot, int nodeId) noexcept;
    void               set      (NodeStateSlot slot, int nodeId, int value) noexcept;

    [[nodiscard]] static constexpr int defaultValue(NodeStateSlot slot) noexcept
    {
        return defaultNodeStateSlotValue(slot);
    }

    [[nodiscard]] static constexpr int indexOf(NodeStateSlot slot, int row) noexcept
    {
        return row * slotCount + static_cast<int>(slot);
    }

    static constexpr std::array<int, slotCount> defaultRowValues = [] {
        std::array<int, slotCount> defaults {};
        for (int slot = 0; slot < slotCount; ++slot) {
            defaults[static_cast<std::size_t>(slot)] = defaultNodeStateSlotValue(static_cast<NodeStateSlot>(slot));
        }
        return defaults;
    }();

    [[nodiscard]] bool isPrepared() const noexcept { return !values.empty(); }

private:

    [[nodiscard]] bool isAddressable(int nodeId) const noexcept;

    std::vector<int>           values;
    NodeRowMap                 rows;
    std::array<int, slotCount> outOfRangeSinks = defaultRowValues;
};
