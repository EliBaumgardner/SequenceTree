#include "AudioUIBridge.h"

void AudioUIBridge::highlightNode(const RTNode& node, bool shouldHighlight)
{
    const auto scope = highlightFifo.write(1);
    if (scope.blockSize1 > 0)
        highlightBuffer[static_cast<size_t>(scope.startIndex1)] = { node.nodeID, shouldHighlight };
    else if (scope.blockSize2 > 0)
        highlightBuffer[static_cast<size_t>(scope.startIndex2)] = { node.nodeID, shouldHighlight };
    else
        jassertfalse;
}

void AudioUIBridge::pushProgress(int parentNodeId, int childNodeId, int durationMs, int graphId)
{
    const auto scope = progressFifo.write(1);
    if (scope.blockSize1 > 0)
        progressBuffer[static_cast<size_t>(scope.startIndex1)] = { parentNodeId, childNodeId, durationMs, graphId };
    else if (scope.blockSize2 > 0)
        progressBuffer[static_cast<size_t>(scope.startIndex2)] = { parentNodeId, childNodeId, durationMs, graphId };
    else
        jassertfalse;
}

void AudioUIBridge::pushCount(int nodeId, int currentCount, int countLimit)
{
    const auto scope = countFifo.write(1);
    if (scope.blockSize1 > 0)
        countBuffer[static_cast<size_t>(scope.startIndex1)] = { nodeId, currentCount, countLimit };
    else if (scope.blockSize2 > 0)
        countBuffer[static_cast<size_t>(scope.startIndex2)] = { nodeId, currentCount, countLimit };
}
