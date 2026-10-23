#pragma once

#include "../Graph/RTData.h"
#include "../Script/ScriptCompiler.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

/*
    The hand-off of everything the audio thread reads from the message thread that builds it.
    Playback reads the graph's nodes and the compiled traversal script from one immutable
    Snapshot behind an atomic pointer: beginBlock loads that pointer once per block and endBlock
    ends the block, and the audio thread never writes to a snapshot.

    To change what playback sees, a caller takes a copy with beginEdit, replaces one member and
    calls publish. publishGraph merges a rebuilt graph into the published nodes, drops that
    graph's stale nodes and sets firstUnlinkedRootId, the root the audio thread starts walking
    from. publishScript publishes a compiled script, and publishActiveTraversalRule compiles the
    active rule's source first and returns the compile result so its errors can be shown.

    The audio thread must never free a snapshot. blockEpoch is odd while a block runs, so publish
    frees the snapshot it replaces on the spot when the epoch is even, and otherwise parks it in
    retiredSnapshots until a later publish finds the epoch has moved on. releaseRetiredSnapshots
    empties that list once playback has stopped.

    SequenceTreeAudioProcessor owns it and brackets each processBlock with BlockScope.
    RTGraphBuilder publishes graphs, and the rules window publishes scripts. Publishing happens
    on the message thread only.
*/

class TraversalRuleState;

class AudioSnapshotPublisher
{
public:

    struct Snapshot;

    explicit AudioSnapshotPublisher(TraversalRuleState& traversalRuleState);

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
    void publishGraph (int graphId, NodeMap graphNodes);

    void releaseRetiredSnapshots();

    ScriptCompileResult publishActiveTraversalRule();

    std::shared_ptr<Snapshot> beginEdit() const;

    const Snapshot* getPublished() const { return publishedSnapshot.get(); }

    struct Snapshot
    {
        std::shared_ptr<NodeMap>  globalNodes;
        std::shared_ptr<RTScript> traversalScript;

        std::uint64_t generation = 0;
    };

private:

    void collectRetiredSnapshots();
    static int findFirstUnlinkedRootId(const NodeMap& nodes);

    struct RetiredSnapshot
    {
        std::shared_ptr<Snapshot> snapshot;
        std::uint64_t             retiredAtEpoch = 0;
    };

    TraversalRuleState& traversalRuleState;

    std::atomic<Snapshot*>     currentSnapshot { nullptr };
    std::atomic<std::uint64_t> blockEpoch { 0 };

    std::shared_ptr<Snapshot>    publishedSnapshot;
    std::vector<RetiredSnapshot> retiredSnapshots;

    std::uint64_t snapshotGeneration = 0;
};
