#include "NodeStateTable.h"

#include <algorithm>
#include <cassert>

int NodeStateTable::defaultValue(NodeStateSlot slot)
{
    switch (slot) {
        case NodeStateSlot::ActiveAlternative:
        case NodeStateSlot::LastNode:
            return -1;

        default:
            return 0;
    }
}

int NodeStateTable::indexOf(NodeStateSlot slot, int nodeId)
{
    return static_cast<int>(slot) * maxNodeIds + nodeId;
}

void NodeStateTable::prepare()
{
    if (values.size() == static_cast<std::size_t>(slotCount) * maxNodeIds) {
        return;
    }

    values.assign(static_cast<std::size_t>(slotCount) * maxNodeIds, 0);

    clear();
}

void NodeStateTable::clear()
{
    if (values.empty()) {
        return;
    }

    for (int slot = 0; slot < slotCount; ++slot) {
        const auto begin = values.begin() + static_cast<std::ptrdiff_t>(slot) * maxNodeIds;

        std::fill(begin, begin + maxNodeIds, defaultValue(static_cast<NodeStateSlot>(slot)));
    }
}

bool NodeStateTable::isAddressable(int nodeId) const
{
    if (values.empty()) {
        return false;
    }

    assert(inRange(nodeId) && "node id exceeded NodeStateTable::maxNodeIds");

    return inRange(nodeId);
}

int NodeStateTable::get(NodeStateSlot slot, int nodeId) const
{
    if (!isAddressable(nodeId)) {
        return defaultValue(slot);
    }

    return values[static_cast<std::size_t>(indexOf(slot, nodeId))];
}

void NodeStateTable::set(NodeStateSlot slot, int nodeId, int value)
{
    if (!isAddressable(nodeId)) {
        return;
    }

    values[static_cast<std::size_t>(indexOf(slot, nodeId))] = value;
}

int NodeStateTable::increment(NodeStateSlot slot, int nodeId)
{
    if (!isAddressable(nodeId)) {
        return defaultValue(slot);
    }

    return ++values[static_cast<std::size_t>(indexOf(slot, nodeId))];
}

int& NodeStateTable::ref(NodeStateSlot slot, int nodeId)
{
    if (!isAddressable(nodeId)) {
        outOfRangeSink = defaultValue(slot);
        return outOfRangeSink;
    }

    return values[static_cast<std::size_t>(indexOf(slot, nodeId))];
}
