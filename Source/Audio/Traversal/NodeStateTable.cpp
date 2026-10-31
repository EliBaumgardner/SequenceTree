#include "NodeStateTable.h"

#include <algorithm>
#include <cassert>

void NodeStateTable::prepare()
{
    if (values.size() == valueCount) {
        clear();
        return;
    }

    values.resize(valueCount);
    rows.prepare();

    for (int row = 0; row < maxNodeIds; ++row) {
        const std::size_t offset = static_cast<std::size_t>(row * slotCount);
        std::copy_n(defaultRowValues.data(), slotCount, values.data() + offset);
    }

    outOfRangeSinks = defaultRowValues;
}

void NodeStateTable::clear() noexcept
{
    if (values.empty()) {
        return;
    }

    for (int row = 0; row < rows.getRowCount(); ++row) {
        const std::size_t offset = static_cast<std::size_t>(row * slotCount);
        std::copy_n(defaultRowValues.data(), slotCount, values.data() + offset);
    }

    rows.clear();
    outOfRangeSinks = defaultRowValues;
}

int NodeStateTable::increment(NodeStateSlot slot, int nodeId) noexcept
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

int NodeStateTable::get(NodeStateSlot slot, int nodeId) const noexcept
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

int& NodeStateTable::ref(NodeStateSlot slot, int nodeId) noexcept
{
    const auto slotIndex = static_cast<std::size_t>(slot);

    if (!isAddressable(nodeId)) {
        outOfRangeSinks[slotIndex] = defaultValue(slot);
        return outOfRangeSinks[slotIndex];
    }

    const int row = rows.claim(nodeId);

    assert(row >= 0 && "more than NodeStateTable::maxNodeIds distinct nodes in one traversal");

    if (row < 0) {
        outOfRangeSinks[slotIndex] = defaultValue(slot);
        return outOfRangeSinks[slotIndex];
    }

    return values[static_cast<std::size_t>(indexOf(slot, row))];
}

void NodeStateTable::set(NodeStateSlot slot, int nodeId, int value) noexcept
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

bool NodeStateTable::isAddressable(int nodeId) const noexcept
{
    if (values.empty()) {
        return false;
    }

    const bool inRange = nodeId >= 0;

    assert(inRange && "node id is negative");

    return inRange;
}
