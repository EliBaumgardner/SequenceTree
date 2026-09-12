#pragma once

#include <cstddef>
#include <vector>

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
    Total
};

class NodeStateTable
{
public:

    static constexpr int         slotCount  = static_cast<int>(NodeStateSlot::Total);
    static constexpr int         maxNodeIds = 1024;
    static constexpr std::size_t valueCount = static_cast<std::size_t>(slotCount) * maxNodeIds;

    void prepare();
    void clear();

    int  get      (NodeStateSlot slot, int nodeId) const;
    void set      (NodeStateSlot slot, int nodeId, int value);
    int  increment(NodeStateSlot slot, int nodeId);
    int& ref      (NodeStateSlot slot, int nodeId);

    static int defaultValue(NodeStateSlot slot);

private:

    static int indexOf(NodeStateSlot slot, int nodeId);

    bool isAddressable(int nodeId) const;

    std::vector<int> values;

    int outOfRangeSink = 0;
};
