#include "NodeStateTable.h"

#include <cassert>

void NodeStateTable::prepare()
{
    if (values.size() == valueCount) {
        return;
    }

    values.resize(valueCount);
    rows.prepare();

    for (int row = 0; row < maxNodeIds; ++row) {
        for (int slot = 0; slot < slotCount; ++slot) {
            values[static_cast<std::size_t>(indexOf(static_cast<NodeStateSlot>(slot), row))] = defaultValue(static_cast<NodeStateSlot>(slot));
        }
    }
}

void NodeStateTable::clear()
{
    if (values.empty()) {
        return;
    }

    for (int row = 0; row < rows.rowCount; ++row) {
        for (int slot = 0; slot < slotCount; ++slot) {
            values[static_cast<std::size_t>(indexOf(static_cast<NodeStateSlot>(slot), row))] = defaultValue(static_cast<NodeStateSlot>(slot));
        }
    }

    rows.clear();
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

bool NodeStateTable::isAddressable(int nodeId) const
{
    if (values.empty()) {
        return false;
    }

    const bool inRange = nodeId >= 0;

    assert(inRange && "node id is negative");

    return inRange;
}

int NodeStateTable::indexOf(NodeStateSlot slot, int row)
{
    return row * slotCount + static_cast<int>(slot);
}
