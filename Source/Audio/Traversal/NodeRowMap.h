#pragma once

#include <vector>

/*
    A set of node ids that numbers its members in the order they join. The first id added is row
    0, the next row 1, and so on; an id keeps its row until the set is emptied, so the rows in use
    are always exactly 0 to rowCount - 1. That lets any id, however large, stand for a small index
    into a caller's own fixed-size arrays.

    claim adds an id if it is not already a member and returns its row. find returns the row of an
    id that is already a member, or -1, and never adds one. clear empties the set, so the next
    claim is row 0 again. keyOfRow lists the members' slots in the order they joined.

    NodeStateTable keeps each node's values in that node's row. TraversalDispatcher's chordVisits
    ignores the rows and uses the set only to remember which nodes one chord has already reached.

    It holds at most maxRows ids; once full, claim returns -1 for a new id, and a negative id is
    never a member. prepare reserves all of its memory, so every other call is safe on the audio
    thread, and clear takes time in proportion to the members, not to maxRows.
*/

class NodeRowMap
{
public:

    void prepare();
    void clear  ();

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
