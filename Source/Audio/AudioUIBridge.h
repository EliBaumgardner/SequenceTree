#pragma once

#include "../Util/PluginModules.h"
#include "../Graph/RTData.h"
#include <array>

template <typename Command, int Capacity = 512>
class CommandFifo
{
public:

    void push(const Command& command)
    {
        const auto scope = fifo.write(1);

        if (scope.blockSize1 > 0) {
            buffer[static_cast<size_t>(scope.startIndex1)] = command;
        }
        else if (scope.blockSize2 > 0) {
            buffer[static_cast<size_t>(scope.startIndex2)] = command;
        }
        else {
            jassertfalse;
        }
    }

    template <typename ApplyCommand>
    void drain(ApplyCommand&& apply)
    {
        const auto scope = fifo.read(fifo.getNumReady());

        for (int i = 0; i < scope.blockSize1; ++i) {
            apply(buffer[static_cast<size_t>(scope.startIndex1 + i)]);
        }

        for (int i = 0; i < scope.blockSize2; ++i) {
            apply(buffer[static_cast<size_t>(scope.startIndex2 + i)]);
        }
    }

    bool hasPending() const { return fifo.getNumReady() > 0; }

private:

    juce::AbstractFifo             fifo { Capacity };
    std::array<Command, Capacity>  buffer {};
};

class AudioUIBridge
{
public:

    struct HighlightCommand
    {
        int  nodeId          = 0;
        bool shouldHighlight = false;
        int  traversalId     = -1;
    };

    struct ProgressCommand
    {
        int  parentNodeId = 0;
        int  childNodeId  = 0;
        int  durationMs   = 0;
        int  graphId      = 0;
        int  traversalId  = -1;
        bool isConnection = false;
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
        int traversalId   = -1;
    };

    CommandFifo<HighlightCommand> highlights;
    CommandFifo<ProgressCommand>  progress;
    CommandFifo<ResetCommand>     arrowResets;
    CommandFifo<CountCommand>     counts;

    bool hasPendingCommands() const
    {
        return highlights.hasPending()
            || progress.hasPending()
            || arrowResets.hasPending()
            || counts.hasPending();
    }

    void highlightNode(int nodeId, bool shouldHighlight, int traversalId = -1)
    {
        highlights.push({ nodeId, shouldHighlight, traversalId });
    }

    void highlightNode(const RTNode& node, bool shouldHighlight, int traversalId = -1)
    {
        highlightNode(node.nodeID, shouldHighlight, traversalId);
    }

    void pushProgress(int parentNodeId, int childNodeId, int durationMs, int graphId, int traversalId, bool isConnection = false)
    {
        progress.push({ parentNodeId, childNodeId, durationMs, graphId, traversalId, isConnection });
    }

    void pushArrowReset(int rootId, int traversalId = -1)
    {
        arrowResets.push({ rootId, traversalId });
    }

    void pushCount(int nodeId, int currentCount, int countLimit)
    {
        counts.push({ nodeId, currentCount, countLimit });
    }
};
