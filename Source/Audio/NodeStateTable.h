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
    SwitchCandidate,
    Total
};

class NodeRowMap
{
public:

    void prepare();
    void clear();

    int find (int nodeId) const;
    int claim(int nodeId);

    static constexpr int maxRows     = 1024;
    static constexpr int keyCapacity = maxRows * 2;
    static constexpr int emptyKey    = -1;

    std::vector<int> keys;
    std::vector<int> rowOfKey;
    std::vector<int> keyOfRow;

    int rowCount = 0;
};

class NodeStateTable
{
public:

    static constexpr int         slotCount  = static_cast<int>(NodeStateSlot::Total);
    static constexpr int         maxNodeIds = NodeRowMap::maxRows;
    static constexpr std::size_t valueCount = static_cast<std::size_t>(slotCount) * maxNodeIds;

    void prepare();
    void clear();

    int  get      (NodeStateSlot slot, int nodeId) const;
    void set      (NodeStateSlot slot, int nodeId, int value);
    int  increment(NodeStateSlot slot, int nodeId);
    int& ref      (NodeStateSlot slot, int nodeId);

    static int defaultValue(NodeStateSlot slot);

private:

    static int indexOf(NodeStateSlot slot, int row);

    bool isAddressable(int nodeId) const;

    std::vector<int> values;
    NodeRowMap       rows;

    int outOfRangeSink = 0;
};
