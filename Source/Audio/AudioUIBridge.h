#pragma once

#include "../Util/PluginModules.h"
#include "../Graph/RTData.h"
#include <array>

class AudioUIBridge
{
public:

    struct HighlightCommand
    {
        int  nodeId          = 0;
        bool shouldHighlight = false;
    };

    struct ProgressCommand
    {
        int parentNodeId = 0;
        int childNodeId  = 0;
        int durationMs   = 0;
        int graphId      = 0;
    };

    struct CountCommand
    {
        int nodeId       = 0;
        int currentCount = 0;
        int countLimit   = 1;
    };

    struct ResetCommand
    {
        int rootId        = 0;
    };


    static constexpr int kHighlightFifoSize = 512;
    juce::AbstractFifo                                highlightFifo { kHighlightFifoSize };
    std::array<HighlightCommand, kHighlightFifoSize>  highlightBuffer {};

    static constexpr int kProgressFifoSize = 512;
    juce::AbstractFifo                                progressFifo { kProgressFifoSize };
    std::array<ProgressCommand, kProgressFifoSize>    progressBuffer {};

    static constexpr int kArrowResetSize = 512;
    juce::AbstractFifo                                arrowResetFifo { kArrowResetSize };
    std::array<ResetCommand,kArrowResetSize>          arrowResetBuffer {};

    static constexpr int kCountFifoSize = 512;
    juce::AbstractFifo                             countFifo { kCountFifoSize };
    std::array<CountCommand, kCountFifoSize>       countBuffer {};


    void highlightNode(const RTNode& node, bool shouldHighlight);
    void pushProgress(int parentNodeId, int childNodeId, int durationMs, int graphId);
    void pushArrowReset(int rootId);
    void pushCount(int nodeId, int currentCount, int countLimit);
};
