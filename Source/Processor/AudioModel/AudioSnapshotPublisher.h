#pragma once

#include "Core/Model/RTData.h"
#include "Processor/Document/TraversalRuleState.h"
#include "Core/Script/ScriptCompiler.h"

#include <juce_data_structures/juce_data_structures.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

/*
    The hand-off of everything the audio thread reads from the message thread that builds it.
    Playback reads the graph's nodes and the compiled traversal script from one immutable
    Snapshot (RTData.h) behind an atomic pointer: beginBlock loads that pointer once per block and
    endBlock ends the block, and the audio thread never writes to a snapshot.

    To change what playback sees, a caller takes a copy with beginEdit, replaces one member and
    calls publish. publishGraphs merges rebuilt graphs into the published nodes in one snapshot,
    drops those graphs' stale nodes and sets startRootId, the root the audio thread starts walking
    from: the lowest-id root no arrow points into, or the lowest-id root when every root has one.
    publishScript publishes a compiled script, and publishActiveTraversalRule compiles the active
    rule's source, publishes it if it compiled and returns the compile result.

    It listens to the rule state's rules and folders, so a rule deleted, restored by undo or made
    live reaches the audio thread whether or not the rules window is open. After a change it
    publishes only when the active rule's id or source differs from the last one it compiled: at
    once for any change other than typing, and 250 ms after typing in the active rule pauses.
    Renaming, moving or adding rules and folders, and typing in another rule, publish nothing, so
    they never reset the variables of the script that is playing. The rules window only compiles
    the text to show errors.

    The audio thread must never free a snapshot. blockEpoch is odd while a block runs, so publish
    frees the snapshot it replaces on the spot when the epoch is even, and otherwise parks it in
    retiredSnapshots until a later publish finds the epoch has moved on. releaseRetiredSnapshots
    empties that list once playback has stopped.

    SequenceTreeAudioProcessor owns it and brackets each processBlock with BlockScope.
    RTGraphBuilder publishes graphs, and the publisher itself republishes the active rule.
    Publishing happens on the message thread only.
*/

class AudioSnapshotPublisher : private juce::ValueTree::Listener,
                               private juce::Timer
{
public:

    explicit AudioSnapshotPublisher(TraversalRuleState& traversalRuleState);
    ~AudioSnapshotPublisher() override;

    const Snapshot* beginBlock()
    {
        blockEpoch.fetch_add(1, std::memory_order_seq_cst);
        return currentSnapshot.load(std::memory_order_seq_cst);
    }

    void endBlock()
    {
        blockEpoch.fetch_add(1, std::memory_order_seq_cst);
    }

    void publish      (std::shared_ptr<Snapshot> snapshot);
    void publishScript(std::shared_ptr<RTScript> script);
    void publishGraphs(std::vector<GraphUpdate> updates);

    void releaseRetiredSnapshots();

    ScriptCompileResult publishActiveTraversalRule();

    std::shared_ptr<Snapshot> beginEdit() const;

    const Snapshot* getPublished() const { return publishedSnapshot.get(); }

private:

    void valueTreeChildAdded       (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved     (juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged  (juce::ValueTree& tree, const juce::Identifier& property) override;

    void timerCallback () override;

    void publishChangedTraversalRule();
    void collectRetiredSnapshots    ();

    static int findStartRootId(const NodeMap& nodes);

    struct RetiredSnapshot
    {
        std::shared_ptr<Snapshot> snapshot;

        std::uint64_t retiredAtEpoch = 0;
    };

    static constexpr int sourceDebounceMs = 250;

    TraversalRuleState& traversalRuleState;

    std::atomic<Snapshot*>     currentSnapshot { nullptr };
    std::atomic<std::uint64_t> blockEpoch { 0 };

    std::shared_ptr<Snapshot> publishedSnapshot;

    std::vector<RetiredSnapshot> retiredSnapshots;

    std::uint64_t snapshotGeneration = 0;

    int compiledRuleId = TraversalRuleState::noActiveRuleId;

    juce::String compiledRuleSource;
};
