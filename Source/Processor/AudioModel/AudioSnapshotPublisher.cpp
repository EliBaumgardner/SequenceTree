#include "AudioSnapshotPublisher.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <algorithm>
#include <iterator>
#include <ranges>

AudioSnapshotPublisher::AudioSnapshotPublisher(TraversalRuleState& traversalRuleState)
    : traversalRuleState(traversalRuleState)
{
    traversalRuleState.rules.addListener(this);
    traversalRuleState.folders.addListener(this);
}

AudioSnapshotPublisher::~AudioSnapshotPublisher()
{
    stopTimer();

    traversalRuleState.rules.removeListener(this);
    traversalRuleState.folders.removeListener(this);
}

void AudioSnapshotPublisher::publish(std::shared_ptr<Snapshot> snapshot)
{
    static_assert(std::atomic<Snapshot*>::is_always_lock_free, "the audio thread must be able to read the snapshot without a lock");

    snapshot->generation = ++snapshotGeneration;

    Snapshot* raw = snapshot.get();

    auto retired      = std::move(publishedSnapshot);
    publishedSnapshot = std::move(snapshot);

    currentSnapshot.store(raw, std::memory_order_seq_cst);

    const std::uint64_t retiredAtEpoch = blockEpoch.load(std::memory_order_seq_cst);

    const bool audioThreadInsideBlock = retiredAtEpoch % 2 != 0;

    if (retired != nullptr && audioThreadInsideBlock) {
        retiredSnapshots.push_back({ std::move(retired), retiredAtEpoch });
    }

    collectRetiredSnapshots();
}

void AudioSnapshotPublisher::publishScript(std::shared_ptr<RTScript> script)
{
    auto edit = beginEdit();

    edit->traversalScript = std::move(script);

    publish(std::move(edit));
}

void AudioSnapshotPublisher::publishGraphs(std::vector<GraphUpdate> updates)
{
    auto edit = beginEdit();

    auto merged = std::make_shared<NodeMap>();

    std::unordered_set<int> replacedGraphIds;

    std::vector<RTNode> replacementNodes;

    for (GraphUpdate& update : updates) {
        replacedGraphIds.insert(update.graphId);

        std::ranges::move(update.nodes.sortedById, std::back_inserter(replacementNodes));
    }

    std::ranges::sort(replacementNodes, {}, &RTNode::nodeID);

    if (edit->globalNodes == nullptr) {
        merged->sortedById = std::move(replacementNodes);
    }
    else {
        auto isOutsideGraphs = [&replacedGraphIds](const RTNode& node) { return ! replacedGraphIds.contains(node.graphID); };

        const std::size_t mergedCapacity = edit->globalNodes->sortedById.size() + replacementNodes.size();

        merged->sortedById.reserve(mergedCapacity);

        std::ranges::set_union(replacementNodes,
                               edit->globalNodes->sortedById | std::views::filter(isOutsideGraphs),
                               std::back_inserter(merged->sortedById),
                               {}, &RTNode::nodeID, &RTNode::nodeID);
    }

    merged->startRootId = findStartRootId(*merged);

    edit->globalNodes = std::move(merged);

    publish(std::move(edit));
}

void AudioSnapshotPublisher::releaseRetiredSnapshots()
{
    retiredSnapshots.clear();
}

ScriptCompileResult AudioSnapshotPublisher::publishActiveTraversalRule()
{
    compiledRuleId     = traversalRuleState.activeRuleId;
    compiledRuleSource = traversalRuleState.activeRuleSource();

    ScriptCompileResult result = compileTraversalScript(compiledRuleSource.toStdString());

    if (result.succeeded()) {
        publishScript(std::make_shared<RTScript>(std::move(result.script)));
    }

    return result;
}

std::shared_ptr<Snapshot> AudioSnapshotPublisher::beginEdit() const
{
    auto edit = std::make_shared<Snapshot>();

    if (publishedSnapshot != nullptr) {
        edit->globalNodes       = publishedSnapshot->globalNodes;
        edit->traversalScript   = publishedSnapshot->traversalScript;
    }

    return edit;
}

void AudioSnapshotPublisher::valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child)
{
    publishChangedTraversalRule();
}

void AudioSnapshotPublisher::valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex)
{
    publishChangedTraversalRule();
}

void AudioSnapshotPublisher::valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex)
{
    publishChangedTraversalRule();
}

void AudioSnapshotPublisher::valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property)
{
    const int changedRuleId = tree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);
    const int liveRuleId    = traversalRuleState.activeRuleId;

    if (property != ValueTreeIdentifiers::RuleSource) {
        publishChangedTraversalRule();

        return;
    }

    if (changedRuleId == liveRuleId) {
        startTimer(sourceDebounceMs);
    }
}

void AudioSnapshotPublisher::timerCallback()
{
    stopTimer();

    publishChangedTraversalRule();
}

void AudioSnapshotPublisher::publishChangedTraversalRule()
{
    const int liveRuleId = traversalRuleState.activeRuleId;

    const juce::String liveRuleSource = traversalRuleState.activeRuleSource();

    if (liveRuleId != compiledRuleId || liveRuleSource != compiledRuleSource) {
        publishActiveTraversalRule();
    }
}

void AudioSnapshotPublisher::collectRetiredSnapshots()
{
    const std::uint64_t epoch = blockEpoch.load(std::memory_order_seq_cst);

    auto isUnreachableByAudioThread = [epoch](const RetiredSnapshot& entry) {
        return epoch > entry.retiredAtEpoch;
    };

    std::erase_if(retiredSnapshots, isUnreachableByAudioThread);
}

int AudioSnapshotPublisher::findStartRootId(const NodeMap& nodes)
{
    std::vector<int> linkedRootIds;

    for (const RTNode& node : nodes.sortedById) {
        for (const RTConnection& connection : node.connections) {
            const int childId = connection.childId;

            const RTNode* const childNode = nodes.find(childId);

            if (childNode != nullptr && childNode->nodeID == childNode->graphID) {
                linkedRootIds.push_back(childId);
            }
        }
    }

    std::ranges::sort(linkedRootIds);

    for (const RTNode& node : nodes.sortedById) {
        if (node.nodeID == node.graphID && !std::ranges::binary_search(linkedRootIds, node.nodeID)) {
            return node.nodeID;
        }
    }

    for (const RTNode& node : nodes.sortedById) {
        if (node.nodeID == node.graphID) {
            return node.nodeID;
        }
    }

    return -1;
}
