#pragma once

#include <cstddef>
#include <vector>

/*
    NodeRowMap exists so that a traversal can remember things about every node it passes through
    (how many times it has left the node, which child it chose last, ...) without allocating while
    audio plays. NodeStateTable keeps those values in a block of memory reserved before playback,
    one row per node, and NodeRowMap says which row belongs to which node.

    It is a set of node ids in which each member has a row number: the first id added is row 0, the
    next new id is row 1, and so on, up to 1024 rows. Ids are arbitrary whole numbers, so they
    cannot be positions in that block; row numbers can.

    claim adds an id and returns its row; an id already in the set keeps its row, and -1 comes back
    when all 1024 rows are taken. find returns the row of an id in the set, or -1, without adding
    it. clear empties the set, so the next id added is row 0 again.

    TraversalDispatcher's chordVisits ignores the rows and uses the set only to remember which
    nodes one chord has reached.
*/

class NodeRowMap
{
public:

    static constexpr int maxRows     = 1024;
    static constexpr int keyCapacity = maxRows * 2;
    static constexpr int emptyKey    = -1;

    void prepare();
    void clear  () noexcept;

    [[nodiscard]] int  find    (int nodeId) const noexcept;
    [[nodiscard]] int  claim   (int nodeId) noexcept;
    [[nodiscard]] bool contains(int nodeId) const noexcept { return find(nodeId) >= 0; }

    [[nodiscard]] int  getRowCount() const noexcept { return rowCount; }
    [[nodiscard]] bool isPrepared () const noexcept { return !keys.empty(); }

private:

    [[nodiscard]] std::size_t findSlot(int nodeId) const noexcept;

    std::vector<int> keys;
    std::vector<int> rowOfKey;
    std::vector<int> keyOfRow;

    int rowCount = 0;
};
