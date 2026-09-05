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

    struct Snapshot
    {
        std::shared_ptr<NodeMap>      globalNodes;
        std::shared_ptr<RTScript>     selectChildScript;

        std::uint64_t                 generation = 0;
    };

    explicit AudioSnapshotPublisher(TraversalRuleState& traversalRuleState);

    const Snapshot* acquireForBlock() const
    {
        return currentSnapshot.load(std::memory_order_acquire);
    }

    void blockCompleted()
    {
        blocksCompleted.fetch_add(1, std::memory_order_release);
    }

    const Snapshot* getPublished() const { return publishedSnapshot.get(); }

    std::shared_ptr<Snapshot> beginEdit() const;

    void publish(std::shared_ptr<Snapshot> snapshot);

    void publishGraph (int graphId, NodeMap graphNodes);
    void publishScript(std::shared_ptr<RTScript> script);

    ScriptCompileResult publishActiveTraversalRule();

    void releaseRetiredSnapshots();

private:

    struct RetiredSnapshot
    {
        std::shared_ptr<Snapshot> snapshot;
        std::uint64_t             retiredAtBlock = 0;
    };

    void collectRetiredSnapshots();

    TraversalRuleState& traversalRuleState;

    std::atomic<Snapshot*>       currentSnapshot { nullptr };
    std::atomic<std::uint64_t>   blocksCompleted { 0 };

    std::shared_ptr<Snapshot>    publishedSnapshot;
    std::vector<RetiredSnapshot> retiredSnapshots;

    std::uint64_t                snapshotGeneration = 0;
};
