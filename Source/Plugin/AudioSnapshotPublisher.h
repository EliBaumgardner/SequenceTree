#pragma once

#include "../Graph/RTData.h"
#include "../Script/ScriptCompiler.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <vector>

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
