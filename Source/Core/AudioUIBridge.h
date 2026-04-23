#pragma once

#include "../Util/PluginModules.h"
#include "../Util/RTData.h"
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

    static constexpr int kHighlightFifoSize = 512;
    juce::AbstractFifo                                highlightFifo { kHighlightFifoSize };
    std::array<HighlightCommand, kHighlightFifoSize>  highlightBuffer {};

    static constexpr int kProgressFifoSize = 512;
    juce::AbstractFifo                                progressFifo { kProgressFifoSize };
    std::array<ProgressCommand, kProgressFifoSize>    progressBuffer {};

    void highlightNode(const RTNode& node, bool shouldHighlight);
    void pushProgress(int parentNodeId, int childNodeId, int durationMs, int graphId);
};
