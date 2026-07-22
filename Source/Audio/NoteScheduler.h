#pragma once

#include "../Util/PluginModules.h"
#include "../Graph/RTData.h"
#include <vector>

class AudioUIBridge;

class NoteScheduler
{
public:

    struct MidiEvent
    {
        int pitch       = 0;
        int velocity    = 0;
        int duration    = 0;
        int midiChannel = 1;
    };

    struct ActiveNote
    {
        MidiEvent        event;
        int              instanceId       = 0;
        int              remainingSamples = 0;
        int              nodeId           = 0;
        RTNode::NodeType nodeType         = RTNode::NodeType::Node;
        bool             isConnectionTrigger = false;
    };

    static constexpr int maxExpectedActiveNotes = 256;

    std::vector<ActiveNote> activeNotes;

    explicit NoteScheduler(AudioUIBridge& bridge);

    void scheduleNote(const RTNode& node, int instanceId, int sample,
                      juce::MidiBuffer& midiMessages,
                      double sampleRate, double tempoMultiplier,
                      int duration, bool isConnectionTrigger = false, int channel = -1, int transpose = 0,
                      double velocityMultiplier = 1.0,
                      int pitchOverride = -1, int velocityOverride = -1);

    void sendNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample);
    void removeNote(int index);
    void clearTraversalNotes(int instanceId);
    void handleOrphanNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages);

    static bool isNodeAudible(RTNode::NodeType nodeType);

private:

    AudioUIBridge& bridge;
};
