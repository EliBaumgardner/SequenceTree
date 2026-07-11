#include "AudioUIBridge.h"

void AudioUIBridge::highlightNode(const RTNode& node, bool shouldHighlight, int traversalId)
{
    const auto scope = highlightFifo.write(1);
    if (scope.blockSize1 > 0) {
        highlightBuffer[static_cast<size_t>(scope.startIndex1)] = { node.nodeID, shouldHighlight, traversalId };
    }
    else if (scope.blockSize2 > 0) {
        highlightBuffer[static_cast<size_t>(scope.startIndex2)] = { node.nodeID, shouldHighlight, traversalId };
    }
    else { jassertfalse; }
}

void AudioUIBridge::pushProgress(int parentNodeId, int childNodeId, int durationMs, int graphId, int traversalId)
{
    const auto scope = progressFifo.write(1);
    if (scope.blockSize1 > 0) {
        progressBuffer[static_cast<size_t>(scope.startIndex1)] = { parentNodeId, childNodeId, durationMs, graphId, traversalId };
    }
    else if (scope.blockSize2 > 0) {
        progressBuffer[static_cast<size_t>(scope.startIndex2)] = { parentNodeId, childNodeId, durationMs, graphId, traversalId };
    }
    else { jassertfalse; }
}

void AudioUIBridge::pushArrowReset(int rootId, int traversalId)
{
    const auto scope = arrowResetFifo.write(1);
    if (scope.blockSize1 > 0) {
        arrowResetBuffer[static_cast<size_t>(scope.startIndex1)] = {rootId, traversalId};
    }
    else if (scope.blockSize2 > 0) {
        arrowResetBuffer[static_cast<size_t>(scope.startIndex2)] = {rootId, traversalId};
    }
    else { jassertfalse; }
}


void AudioUIBridge::pushCount(int nodeId, int currentCount, int countLimit)
{
    const auto scope = countFifo.write(1);
    if (scope.blockSize1 > 0) {
        countBuffer[static_cast<size_t>(scope.startIndex1)] = { nodeId, currentCount, countLimit };
    }
    else if (scope.blockSize2 > 0) {
        countBuffer[static_cast<size_t>(scope.startIndex2)] = { nodeId, currentCount, countLimit };
    }
    else { jassertfalse; }
}
