#include "NodeRowMap.h"

#include <cstdint>

void NodeRowMap::prepare()
{
    if (keys.size() == static_cast<std::size_t>(keyCapacity)) {
        clear();
        return;
    }

    keys.assign(keyCapacity, emptyKey);
    rowOfKey.assign(keyCapacity, 0);
    keyOfRow.assign(maxRows, 0);

    rowCount = 0;
}

void NodeRowMap::clear() noexcept
{
    for (int row = 0; row < rowCount; ++row) {
        const auto slot = static_cast<std::size_t>(keyOfRow[static_cast<std::size_t>(row)]);
        keys[slot] = emptyKey;
    }

    rowCount = 0;
}

std::size_t NodeRowMap::findSlot(int nodeId) const noexcept
{
    std::size_t slot = (static_cast<std::uint32_t>(nodeId) * 2654435761u) & (keyCapacity - 1);

    while (keys[slot] != emptyKey && keys[slot] != nodeId) {
        slot = (slot + 1) & (keyCapacity - 1);
    }

    return slot;
}

int NodeRowMap::find(int nodeId) const noexcept
{
    if (nodeId < 0 || keys.empty()) {
        return -1;
    }

    const std::size_t slot = findSlot(nodeId);

    return (keys[slot] == nodeId) ? rowOfKey[slot] : -1;
}

int NodeRowMap::claim(int nodeId) noexcept
{
    if (nodeId < 0 || keys.empty()) {
        return -1;
    }

    const std::size_t slot = findSlot(nodeId);

    if (keys[slot] == nodeId) {
        return rowOfKey[slot];
    }

    if (rowCount == maxRows) {
        return -1;
    }

    keys    [slot]                                = nodeId;
    rowOfKey[slot]                                = rowCount;
    keyOfRow[static_cast<std::size_t>(rowCount)]  = static_cast<int>(slot);

    rowCount = rowCount + 1;

    return rowCount - 1;
}
