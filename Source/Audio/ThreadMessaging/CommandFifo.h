#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <atomic>
#include <concepts>

/*
    The queue behind every message that crosses the audio boundary: a fixed-size queue of Command
    values that one thread fills and one other thread empties, in the order they were added,
    without either side locking or allocating, so the audio thread can sit on either end without
    being kept waiting.

    push adds a command at the back. When Capacity commands are already waiting, the new one is
    dropped and overflowed is set, and it stays set until the reader clears it, so the reader knows
    it missed some and has to catch up another way. drain hands every waiting command to a
    function, oldest first, and removes them. hasPending says whether any are waiting.

    AudioUIBridge holds three to send highlights, arrow progress and counts to the canvas.
    AudioCommandDrainer drains them every frame and redraws the canvas from scratch when one
    overflowed. TraversalSession's previewRequests carries note preview requests from BottomBar
    and NodeCanvas to the audio thread, and VirtualMidiOutput uses one to hand MIDI to the thread
    that sends it.
*/

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
            overflowed.store(true);
        }
    }

    template <std::invocable<const Command&> ApplyCommand>
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

    constexpr bool hasPending() const
    {
        return fifo.getNumReady() > 0;
    }

    std::atomic<bool> overflowed { false };

private:

    juce::AbstractFifo            fifo { Capacity };
    std::array<Command, Capacity> buffer {};
};
