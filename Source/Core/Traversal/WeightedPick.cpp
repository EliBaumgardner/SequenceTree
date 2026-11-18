#include "WeightedPick.h"
#include "Core/Model/RTData.h"

#include <algorithm>

void WeightedPick::tally(int countLimit, int probability)
{
    const int weight = std::max(probability, 0);

    if (countLimit > maxLimit) {
        maxLimit    = countLimit;
        totalWeight = weight;
    }
    else if (countLimit == maxLimit) {
        totalWeight += weight;
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
    const int weight = std::max(probability, 0);

    if (countLimit != maxLimit) {
        return false;
    }

    runningWeight += weight;

    return pick < runningWeight;
}
