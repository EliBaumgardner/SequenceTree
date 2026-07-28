#pragma once

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
    LastNode
};

class NodeStateTable
{
public:

    static constexpr int slotCount  = 10;
    static constexpr int maxNodeIds = 1024;

    void prepare();
    void clear();

    int  get      (NodeStateSlot slot, int nodeId) const;
    void set      (NodeStateSlot slot, int nodeId, int value);
    int  increment(NodeStateSlot slot, int nodeId);
    int& ref      (NodeStateSlot slot, int nodeId);

    static int defaultValue(NodeStateSlot slot);

private:

    static bool inRange(int nodeId) { return nodeId >= 0 && nodeId < maxNodeIds; }
    static int  indexOf(NodeStateSlot slot, int nodeId);

    bool isAddressable(int nodeId) const;

    std::vector<int> values;

    int outOfRangeSink = 0;
};
