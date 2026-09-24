#include "NodeStateTable.h"

#include <algorithm>
#include <cassert>
#include <cstdint>

void NodeRowMap::prepare()
{
    keys.assign(keyCapacity, emptyKey);
    rowOfKey.assign(keyCapacity, 0);
    keyOfRow.assign(maxRows, 0);

    rowCount = 0;
}

void NodeRowMap::clear()
{
    for (int row = 0; row < rowCount; ++row) {
        keys[static_cast<std::size_t>(keyOfRow[static_cast<std::size_t>(row)])] = emptyKey;
    }

    rowCount = 0;
}

int NodeRowMap::find(int nodeId) const
{
    if (nodeId < 0 || keys.empty()) {
        return -1;
    }

    int key = static_cast<int>((static_cast<std::uint32_t>(nodeId) * 2654435761u) & (keyCapacity - 1));

    while (keys[static_cast<std::size_t>(key)] != emptyKey) {
        if (keys[static_cast<std::size_t>(key)] == nodeId) {
            return rowOfKey[static_cast<std::size_t>(key)];
        }

        key = (key + 1) & (keyCapacity - 1);
    }

    return -1;
}

int NodeRowMap::claim(int nodeId)
{
    if (nodeId < 0 || keys.empty()) {
        return -1;
    }

    int key = static_cast<int>((static_cast<std::uint32_t>(nodeId) * 2654435761u) & (keyCapacity - 1));

    while (keys[static_cast<std::size_t>(key)] != emptyKey) {
        if (keys[static_cast<std::size_t>(key)] == nodeId) {
            return rowOfKey[static_cast<std::size_t>(key)];
        }

        key = (key + 1) & (keyCapacity - 1);
    }

    if (rowCount == maxRows) {
        return -1;
    }

    keys    [static_cast<std::size_t>(key)]      = nodeId;
    rowOfKey[static_cast<std::size_t>(key)]      = rowCount;
    keyOfRow[static_cast<std::size_t>(rowCount)] = key;

    rowCount = rowCount + 1;

    return rowCount - 1;
}

int NodeStateTable::defaultValue(NodeStateSlot slot)
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

int NodeStateTable::indexOf(NodeStateSlot slot, int row)
{
    return static_cast<int>(slot) * maxNodeIds + row;
}

void NodeStateTable::prepare()
{
    if (values.size() == valueCount) {
        return;
    }

    values.resize(valueCount);
    rows.prepare();

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

    rows.clear();
}

bool NodeStateTable::isAddressable(int nodeId) const
{
    if (values.empty()) {
        return false;
    }

    const bool inRange = nodeId >= 0;

    assert(inRange && "node id is negative");

    return inRange;
}

int NodeStateTable::get(NodeStateSlot slot, int nodeId) const
{
    if (!isAddressable(nodeId)) {
        return defaultValue(slot);
    }

    const int row = rows.find(nodeId);

    if (row < 0) {
        return defaultValue(slot);
    }

    return values[static_cast<std::size_t>(indexOf(slot, row))];
}

void NodeStateTable::set(NodeStateSlot slot, int nodeId, int value)
{
    if (!isAddressable(nodeId)) {
        return;
    }

    const int row = rows.claim(nodeId);

    assert(row >= 0 && "more than NodeStateTable::maxNodeIds distinct nodes in one traversal");

    if (row < 0) {
        return;
    }

    values[static_cast<std::size_t>(indexOf(slot, row))] = value;
}

int NodeStateTable::increment(NodeStateSlot slot, int nodeId)
{
    if (!isAddressable(nodeId)) {
        return defaultValue(slot);
    }

    const int row = rows.claim(nodeId);

    assert(row >= 0 && "more than NodeStateTable::maxNodeIds distinct nodes in one traversal");

    if (row < 0) {
        return defaultValue(slot);
    }

    return ++values[static_cast<std::size_t>(indexOf(slot, row))];
}

int& NodeStateTable::ref(NodeStateSlot slot, int nodeId)
{
    if (!isAddressable(nodeId)) {
        outOfRangeSink = defaultValue(slot);
        return outOfRangeSink;
    }

    const int row = rows.claim(nodeId);

    assert(row >= 0 && "more than NodeStateTable::maxNodeIds distinct nodes in one traversal");

    if (row < 0) {
        outOfRangeSink = defaultValue(slot);
        return outOfRangeSink;
    }

    return values[static_cast<std::size_t>(indexOf(slot, row))];
}
