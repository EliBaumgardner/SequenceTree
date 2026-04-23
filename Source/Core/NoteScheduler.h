#pragma once

#include "../Util/PluginModules.h"
#include "../Util/RTData.h"
#include <vector>

class AudioUIBridge;

class NoteScheduler
{
public:

    struct MidiEvent
    {
        int pitch    = 0;
        int velocity = 0;
        int duration = 0;
    };

    struct ActiveNote
    {
        MidiEvent event;
        int       traversalId      = 0;
        int       remainingSamples = 0;
        RTNode    noteNode;
        bool      isConnectionTrigger = false;
    };

    std::vector<ActiveNote> activeNotes;

    explicit NoteScheduler(AudioUIBridge& bridge);

    void scheduleNote(const RTNode& node, int traversalId, int sample,
                      juce::MidiBuffer& midiMessages,
                      double sampleRate, double tempoMultiplier,
                      int duration, bool isConnectionTrigger = false);

    void sendNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample);
    void removeNote(int index);
    void clearTraversalNotes(int traversalId);
    void handleOrphanNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages);

    static bool isNodeAudible(RTNode::NodeType nodeType);

private:

    AudioUIBridge& bridge;
};
