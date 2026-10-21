#include "VirtualMidiOutput.h"
#include "../../Util/NodeInfo.h"
#include <algorithm>

VirtualMidiOutput::VirtualMidiOutput()
{
    const juce::Array<juce::MidiDeviceInfo> existingPorts = juce::MidiInput::getAvailableDevices();
    juce::String                            portName      = JucePlugin_Name;
    int                                     portNumber    = 2;

    while (std::any_of(existingPorts.begin(), existingPorts.end(), [&portName](const juce::MidiDeviceInfo& existing) { return existing.name == portName; })) {
        portName = juce::String(JucePlugin_Name) + " " + juce::String(portNumber++);
    }

    port = juce::MidiOutput::createNewDevice(portName);

    if (port == nullptr) {
        return;
    }

    port->startBackgroundThread();

    startTimer(drainIntervalMs);
}

VirtualMidiOutput::~VirtualMidiOutput()
{
    stopTimer();

    if (port == nullptr) {
        return;
    }

    port->stopBackgroundThread();

    for (int channel = minimumMidiChannel; channel <= maximumMidiChannel; ++channel) {
        port->sendMessageNow(juce::MidiMessage::allNotesOff(channel));
    }
}

void VirtualMidiOutput::mirrorBlock(const juce::MidiBuffer& midiMessages, const double sampleRate) noexcept
{
    const double blockStartMs = juce::Time::getMillisecondCounterHiRes();

    if (port == nullptr) {
        return;
    }

    for (const juce::MidiMessageMetadata metadata : midiMessages) {
        TimedMessage message;

        if (metadata.numBytes > static_cast<int>(message.bytes.size())) {
            continue;
        }

        std::copy_n(metadata.data, metadata.numBytes, message.bytes.begin());

        message.size   = metadata.numBytes;
        message.timeMs = blockStartMs + metadata.samplePosition * millisecondsPerSecond / sampleRate;

        messages.push(message);
    }
}

void VirtualMidiOutput::hiResTimerCallback()
{
    messages.drain([this](const TimedMessage& message) {
        outgoing.clear();

        outgoing.addEvent(message.bytes.data(), message.size, 0);

        port->sendBlockOfMessages(outgoing, message.timeMs, millisecondsPerSecond);
    });
}
