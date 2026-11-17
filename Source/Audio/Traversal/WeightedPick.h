#pragma once

struct WeightedPick
{
    void tally(int countLimit, int probability);

    bool draw   (int randomValue);
    bool accepts(int countLimit, int probability);

    int maxLimit      = 0;
    int totalWeight   = 0;
    int runningWeight = 0;
    int pick          = 0;
};
