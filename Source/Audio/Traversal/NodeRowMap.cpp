#include "NodeRowMap.h"

#include <cstddef>
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
