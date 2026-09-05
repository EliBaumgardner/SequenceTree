#include "AudioSnapshotPublisher.h"
#include "../Graph/TraversalRuleState.h"

#include <algorithm>

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

    if (edit->globalNodes != nullptr) {
        edit->globalNodes = std::make_shared<NodeMap>(*edit->globalNodes);
    }
    else {
        edit->globalNodes = std::make_shared<NodeMap>();
    }

    for (const auto& [nodeId, node] : graphNodes) {
        (*edit->globalNodes)[nodeId] = node;
    }

    std::vector<int> staleIds;

    for (const auto& [nodeId, node] : *edit->globalNodes) {
        if (node->graphID == graphId && !graphNodes.count(nodeId)) {
            staleIds.push_back(nodeId);
        }
    }

    for (int id : staleIds) {
        edit->globalNodes->erase(id);
    }

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
