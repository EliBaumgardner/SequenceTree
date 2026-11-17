#include "WeightedPick.h"
#include "Core/Model/RTData.h"

void WeightedPick::tally(int countLimit, int probability)
{
    if (countLimit > maxLimit) {
        maxLimit    = countLimit;
        totalWeight = probability;
    }
    else if (countLimit == maxLimit) {
        totalWeight += probability;
    }
}

bool WeightedPick::draw(int randomValue)
{
    int selectionSpan = totalWeight;

    if (totalWeight <= 0) {
        return false;
    }

    if (selectionSpan < RTNode::probabilityScale) {
        selectionSpan = RTNode::probabilityScale;
    }

    pick = randomValue % selectionSpan;

    return true;
}

bool WeightedPick::accepts(int countLimit, int probability)
{
    if (countLimit != maxLimit) {
        return false;
    }

    runningWeight += probability;

    return pick < runningWeight;
}
