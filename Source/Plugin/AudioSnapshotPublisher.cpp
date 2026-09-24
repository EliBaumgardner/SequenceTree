#include "AudioSnapshotPublisher.h"
#include "../Graph/TraversalRuleState.h"

#include <algorithm>
#include <iterator>
#include <ranges>

AudioSnapshotPublisher::AudioSnapshotPublisher(TraversalRuleState& traversalRuleState)
    : traversalRuleState(traversalRuleState)
{
}

std::shared_ptr<AudioSnapshotPublisher::Snapshot> AudioSnapshotPublisher::beginEdit() const
{
    auto edit = std::make_shared<Snapshot>();

    if (publishedSnapshot != nullptr) {
        edit->globalNodes       = publishedSnapshot->globalNodes;
        edit->selectChildScript = publishedSnapshot->selectChildScript;
    }

    return edit;
}

void AudioSnapshotPublisher::publishGraph(int graphId, NodeMap graphNodes)
{
    auto edit = beginEdit();

    auto merged = std::make_shared<NodeMap>();

    if (edit->globalNodes == nullptr) {
        merged->sortedById = std::move(graphNodes.sortedById);
    }
    else {
        auto isOutsideGraph = [graphId](const RTNode& node) { return node.graphID != graphId; };

        merged->sortedById.reserve(edit->globalNodes->sortedById.size() + graphNodes.sortedById.size());

        std::ranges::set_union(graphNodes.sortedById,
                               edit->globalNodes->sortedById | std::views::filter(isOutsideGraph),
                               std::back_inserter(merged->sortedById),
                               {}, &RTNode::nodeID, &RTNode::nodeID);
    }

    edit->globalNodes = std::move(merged);

    publish(std::move(edit));
}

void AudioSnapshotPublisher::publishScript(std::shared_ptr<RTScript> script)
{
    auto edit = beginEdit();

    edit->selectChildScript = std::move(script);

    publish(std::move(edit));
}

ScriptCompileResult AudioSnapshotPublisher::publishActiveTraversalRule()
{
    ScriptCompileResult result =
        compileTraversalScript(traversalRuleState.activeRuleSource().toStdString());

    if (result.succeeded()) {
        publishScript(std::make_shared<RTScript>(std::move(result.script)));
    }

    return result;
}

void AudioSnapshotPublisher::publish(std::shared_ptr<Snapshot> snapshot)
{
    static_assert(std::atomic<Snapshot*>::is_always_lock_free,
                  "the audio thread must be able to read the snapshot without a lock");

    snapshot->generation = ++snapshotGeneration;

    Snapshot* raw = snapshot.get();

    auto retired      = std::move(publishedSnapshot);
    publishedSnapshot = std::move(snapshot);

    currentSnapshot.store(raw, std::memory_order_release);

    std::atomic_thread_fence(std::memory_order_seq_cst);

    if (retired != nullptr) {
        retiredSnapshots.push_back({ std::move(retired),
                                     blocksCompleted.load(std::memory_order_acquire) });
    }

    collectRetiredSnapshots();
}

void AudioSnapshotPublisher::collectRetiredSnapshots()
{
    const std::uint64_t completed = blocksCompleted.load(std::memory_order_acquire);

    auto isUnreachableByAudioThread = [completed](const RetiredSnapshot& entry) {
        return completed > entry.retiredAtBlock;
    };

    retiredSnapshots.erase(std::remove_if(retiredSnapshots.begin(),
                                          retiredSnapshots.end(),
                                          isUnreachableByAudioThread),
                           retiredSnapshots.end());
}

void AudioSnapshotPublisher::releaseRetiredSnapshots()
{
    retiredSnapshots.clear();
}
