#pragma once

#include <juce_audio_devices/juce_audio_devices.h>
#include "AudioUIBridge.h"
#include <array>
#include <memory>

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
