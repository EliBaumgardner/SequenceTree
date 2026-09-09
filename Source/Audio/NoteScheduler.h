#pragma once

#include "../Util/PluginModules.h"
#include "../Graph/RTData.h"
#include <vector>

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
        int              runId       = 0;
        double           remainingSamples = 0.0;
        int              nodeId           = 0;
        RTNode::NodeType nodeType         = RTNode::NodeType::Node;
        bool             isConnectionTrigger = false;
    };

    struct NoteVoicing
    {
        int    channel            = -1;
        int    transpose          = 0;
        double velocityMultiplier = 1.0;
        int    pitchOverride      = -1;
        int    velocityOverride   = -1;
    };

    static constexpr int maxExpectedActiveNotes = 1024;

    std::vector<ActiveNote> activeNotes;

    NoteScheduler();

    void scheduleNote(const RTNode& node, int runId, double sample,
                      juce::MidiBuffer& midiMessages,
                      double sampleRate, double tempoMultiplier,
                      int duration, bool isConnectionTrigger,
                      const NoteVoicing& voicing);

    void sendNoteOff(const ActiveNote& note, juce::MidiBuffer& midiMessages, int sample);
    void removeNote(int index);

    static bool isNoteSounding(const ActiveNote& note);

    static bool isNodeAudible(RTNode::NodeType nodeType);
};
