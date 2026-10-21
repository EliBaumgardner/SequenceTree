#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include "CommandFifo.h"
#include <array>
#include <memory>

/*
    A virtual MIDI output port named after the plugin. It sends out a copy of the MIDI each block
    produces, in addition to what the plugin returns to the host, so any other app that reads MIDI
    ports can receive the notes.

    At the end of every processBlock, mirrorBlock copies the block's MIDI, with the time each
    message should play, into a CommandFifo, without locking or allocating. A timer passes those
    messages to the port every millisecond, and the port sends each one at its time. The
    constructor picks a port name no other device is using, and the destructor turns off every
    note on every channel.

    If the port can't be created, it does nothing.
*/

class VirtualMidiOutput : private juce::HighResolutionTimer
{
public:
    VirtualMidiOutput();
    ~VirtualMidiOutput() override;

    void mirrorBlock(const juce::MidiBuffer& midiMessages, const double sampleRate) noexcept;

private:

    void hiResTimerCallback() override;

    struct TimedMessage
    {
        std::array<juce::uint8, 3> bytes  {};
        int                        size   = 0;
        double                     timeMs = 0.0;
    };

    static constexpr int    capacity              = 4096;
    static constexpr int    drainIntervalMs       = 1;
    static constexpr double millisecondsPerSecond = 1000.0;

    std::unique_ptr<juce::MidiOutput>   port;
    CommandFifo<TimedMessage, capacity> messages;
    juce::MidiBuffer                    outgoing;
};
