#include "TraversalSession.h"
#include "EventManager.h"
#include "../Script/ScriptCompiler.h"

#include <algorithm>

namespace {

int homeRootId(const TraversalPool::Instance& instance)
{
    if (instance.runtime.originRootId != -1) {
        return instance.runtime.originRootId;
    }

    return instance.logic.rootId;
}

}

TraversalSession::TraversalSession(EventManager& eventManager) : eventManager(eventManager)
{
    activeRootIdScratch.reserve(scratchCapacity);
    restartRootScratch.reserve(scratchCapacity);
    linkedRootScratch.reserve(scratchCapacity);
    removedRunIdScratch.reserve(maxConcurrentTraversals);
    replayMidi.ensureSize(replayMidiCapacityBytes);
}

void TraversalSession::prepare()
{
    syncedGraphGeneration = 0;
    syncedPoolEpoch       = 0;

    nativeFallbackScript = compileTraversalScript(defaultTraversalScriptSource()).script;
    scriptRule.script = &nativeFallbackScript;

    if (useScriptedChildSelection) {
        traversals.prepare(maxConcurrentTraversals, scriptRule);
    }
    else {
        traversals.prepare(maxConcurrentTraversals, NativeTraversalRule::instance());
    }
}

void TraversalSession::setSelectChildScript(const RTScript* script)
{
    if (script != nullptr && !script->isEmpty()) {
        scriptRule.script = script;
        return;
    }

    scriptRule.script = &nativeFallbackScript;
}

void TraversalSession::beginReplay(const DispatchContext& context, double targetSamples)
{
    silenceAllNotes(context.midiMessages);
    clearTraversals();

    eventManager.bridge.beginRecording();

    playback              = Playback::Replaying;
    replayRemainingSamples = targetSamples;

    const DispatchContext replayContext { context.nodes, context.traversalMap, replayMidi,
                                          context.sampleRate, context.tempoMultiplier,
                                          context.transpose, context.velocityScale };

    replayMidi.clear();
    startTraversalsFromFirstRoot(replayContext);
}

TraversalSession::Playback TraversalSession::continueReplay(const DispatchContext& context,
                                                            std::uint64_t graphGeneration, int numSamples,
                                                            bool playing)
{
    if (playback == Playback::Live) {
        return Playback::Live;
    }

    const DispatchContext replayContext { context.nodes, context.traversalMap, replayMidi,
                                          context.sampleRate, context.tempoMultiplier,
                                          context.transpose, context.velocityScale };

    syncWithGraph(replayContext, graphGeneration);

    const double budgetMs  = 1000.0 * replayShareOfBlock * numSamples / context.sampleRate;
    const double startedMs = juce::Time::getMillisecondCounterHiRes();

    while (replayRemainingSamples >= 1.0
           && juce::Time::getMillisecondCounterHiRes() - startedMs < budgetMs) {
        const int chunkSamples = static_cast<int>(juce::jmin(replayRemainingSamples,
                                                             static_cast<double>(replayChunkSamples)));
        replayMidi.clear();
        eventManager.processEvents(chunkSamples, replayContext);
        replayRemainingSamples -= chunkSamples;

        eventManager.bridge.recordClockMs += 1000.0 * chunkSamples / context.sampleRate;
    }

    if (replayRemainingSamples >= 1.0) {
        if (playing) {
            replayRemainingSamples += numSamples;
        }

        return Playback::Replaying;
    }

    eventManager.bridge.deliverRecording();

    if (playing) {
        for (const auto& note : eventManager.scheduler.activeNotes) {
            if (!NoteScheduler::isNoteSounding(note)) {
                continue;
            }

            context.midiMessages.addEvent(juce::MidiMessage::noteOn(note.event.midiChannel, note.event.pitch,
                                          static_cast<juce::uint8>(note.event.velocity)), 0);
        }
    }

    playback = Playback::Live;
    return Playback::Live;
}

void TraversalSession::silenceAllNotes(juce::MidiBuffer& midiMessages)
{
    for (const auto& note : eventManager.scheduler.activeNotes) {
        eventManager.scheduler.sendNoteOff(note, midiMessages, 0);
    }

    eventManager.bridge.clearAllHighlights();
    eventManager.bridge.pushArrowReset(AudioUIBridge::allTrails);
    eventManager.scheduler.activeNotes.clear();
}

void TraversalSession::clearTraversals()
{
    eventManager.dispatcher.flagScheduler.clear();
    eventManager.bridge.pushArrowReset(AudioUIBridge::allTrails);
    traversals.clear();
}

void TraversalSession::suspendActiveNotes(juce::MidiBuffer& midiMessages)
{
    for (const auto& note : eventManager.scheduler.activeNotes) {
        eventManager.scheduler.sendNoteOff(note, midiMessages, 0);
    }
}

void TraversalSession::restartActiveTraversals(const DispatchContext& context)
{
    restartRootScratch.clear();

    eventManager.dispatcher.flagScheduler.clear();

    for (const auto& [runId, instance] : traversals.entries()) {
        if (instance.runtime.isSpawned()) {
            continue;
        }

        const int rootId = homeRootId(instance);

        if (std::ranges::find(restartRootScratch, rootId) == restartRootScratch.end()) {
            restartRootScratch.push_back(rootId);
        }
    }

    eventManager.bridge.pushArrowReset(AudioUIBridge::allTrails);
    traversals.clear();

    for (int rootId : restartRootScratch) {
        const RTNode* rootNode = context.nodes.find(rootId);
        if (rootNode == nullptr) {
            continue;
        }

        for (const RTtraversal& assigned : rootNode->traversals) {
            startTraversal(*rootNode, assigned, context);
        }
    }
}

void TraversalSession::syncWithGraph(const DispatchContext& context, std::uint64_t graphGeneration)
{
    if (graphGeneration == syncedGraphGeneration
        && traversals.epoch == syncedPoolEpoch) {
        return;
    }

    syncActiveTraversals(context.nodes);
    removeDeletedTraversals(context.nodes, context.midiMessages);
    startMissingTraversals(context);
    syncTraversalLoopLimits(context);

    syncedGraphGeneration = graphGeneration;
    syncedPoolEpoch       = traversals.epoch;
}

void TraversalSession::syncActiveTraversals(const NodeMap& nodes)
{
    for (auto& [runId, instance] : traversals.entries()) {
        TraversalLogic& logic = instance.logic;

        if (instance.runtime.asFlag) {
            const RTNode* flagNode = nodes.find(instance.runtime.sourceNodeId);
            if (flagNode != nullptr && flagNode->flagTraversal.key == logic.traversal.key) {
                logic.traversal = flagNode->flagTraversal;
            }
            continue;
        }

        const RTNode* rootNode = nodes.find(homeRootId(instance));
        if (rootNode == nullptr) {
            continue;
        }

        for (const RTtraversal& assigned : rootNode->traversals) {
            if (assigned.key == logic.traversal.key) {
                logic.traversal = assigned;
                break;
            }
        }
    }
}

void TraversalSession::removeDeletedTraversals(const NodeMap& nodes, juce::MidiBuffer& midiMessages)
{
    removedRunIdScratch.clear();

    for (const auto& [runId, instance] : traversals.entries()) {
        const TraversalLogic& traverser = instance.logic;

        bool stillAssigned = false;
        const RTNode* rootNode      = nodes.find(homeRootId(instance));

        if (rootNode != nullptr) {
            if (instance.runtime.asFlag) {
                stillAssigned = true;
            }
            else {
                for (const RTtraversal& assigned : rootNode->traversals) {
                    if (assigned.key == traverser.traversal.key) {
                        stillAssigned = true;
                        break;
                    }
                }
            }
        }

        if (!stillAssigned) {
            removedRunIdScratch.push_back(runId);
        }
    }

    for (int runId : removedRunIdScratch) {
        stopTraversalNotes(runId, midiMessages);
        traversals.erase(runId);
    }
}

void TraversalSession::startMissingTraversals(const DispatchContext& context)
{
    activeRootIdScratch.clear();

    for (const auto& [runId, instance] : traversals.entries()) {
        if (instance.runtime.isSpawned()) {
            continue;
        }

        if (std::ranges::find(activeRootIdScratch, homeRootId(instance)) == activeRootIdScratch.end()) {
            activeRootIdScratch.push_back(homeRootId(instance));
        }
    }

    auto isActive = [this](int rootId, const TraversalKey& key) {
        for (const auto& [runId, instance] : traversals.entries()) {
            if (homeRootId(instance) == rootId && instance.logic.traversal.key == key) {
                return true;
            }
        }
        return false;
    };

    for (int rootId : activeRootIdScratch) {
        const RTNode* rootNode = context.nodes.find(rootId);
        if (rootNode == nullptr) {
            continue;
        }

        for (const RTtraversal& assigned : rootNode->traversals) {
            if (isActive(rootId, assigned.key)) {
                continue;
            }

            startTraversal(*rootNode, assigned, context);
        }
    }
}

void TraversalSession::syncTraversalLoopLimits(const DispatchContext& context)
{
    for (auto& [runId, instance] : traversals.entries())
    {
        TraversalLogic& traversal = instance.logic;

        const RTNode* rootEntry = context.nodes.find(traversal.rootId);
        if (rootEntry == nullptr) {
            continue;
        }

        const RTNode& rootNode = *rootEntry;

        int newLoopLimit = rootNode.graphLoopLimit;
        if (newLoopLimit == traversal.loop.limit) {
            continue;
        }

        traversal.loop.limit = newLoopLimit;

        if (traversal.state == TraversalLogic::TraversalState::End) {
            if (newLoopLimit == 0 || traversal.loop.count < newLoopLimit) {
                traversal.primary.target = traversal.rootId;
                traversal.state          = TraversalLogic::TraversalState::Active;
                traversal.advanceAlternative(context.nodes, traversal.rootId);

                eventManager.bridge.highlightNode(rootNode, AudioUIBridge::HighlightKind::Show, runId,
                                                  traversal.traversal.key.typeId);
                eventManager.dispatcher.pushNote(rootNode, runId, context, 0);
            }
        }
    }
}

namespace {

bool isRootNode(const RTNode& node)
{
    return node.nodeID == node.graphID;
}

}

int TraversalSession::findFirstUnlinkedRootId(const NodeMap& nodes)
{
    linkedRootScratch.clear();

    for (const RTNode& node : nodes.sortedById) {
        for (const RTConnection& connection : node.connections) {
            const int childId = connection.childId;

            const RTNode* const childNode = nodes.find(childId);

            if (childNode != nullptr && isRootNode(*childNode)) {
                linkedRootScratch.push_back(childId);
            }
        }
    }

    std::ranges::sort(linkedRootScratch);

    for (const RTNode& node : nodes.sortedById) {
        if (isRootNode(node) && !std::ranges::binary_search(linkedRootScratch, node.nodeID)) {
            return node.nodeID;
        }
    }

    return -1;
}

bool TraversalSession::startTraversalsFromFirstRoot(const DispatchContext& context)
{
    const int rootId = findFirstUnlinkedRootId(context.nodes);

    if (rootId == -1) {
        return false;
    }

    const RTNode& rootNode = *context.nodes.find(rootId);

    for (const RTtraversal& traversal : rootNode.traversals) {
        startTraversal(rootNode, traversal, context);
    }

    return true;
}

void TraversalSession::startTraversal(const RTNode& rootNode, const RTtraversal& traversal,
                                      const DispatchContext& context)
{
    const int rootId = rootNode.nodeID;
    const int runId  = traversals.nextRunId();

    TraversalPool::Instance* acquired = traversals.acquire(runId, rootId, traversal);

    if (acquired == nullptr) {
        return;
    }

    acquired->logic.begin(context.nodes, rootId, rootNode.graphLoopLimit);

    eventManager.bridge.highlightNode(rootNode, AudioUIBridge::HighlightKind::Show, runId, traversal.key.typeId);
    eventManager.dispatcher.pushNote(rootNode, runId, context, 0);
}

void TraversalSession::stopTraversalNotes(int runId, juce::MidiBuffer& midiMessages)
{
    auto& activeNotes = eventManager.scheduler.activeNotes;

    for (int i = static_cast<int>(activeNotes.size()) - 1; i >= 0; --i) {
        auto& note = activeNotes[i];

        if (note.runId != runId || note.role != NoteScheduler::NoteRole::Stepping) {
            continue;
        }

        eventManager.scheduler.sendNoteOff(note, midiMessages, 0);

        eventManager.bridge.highlightNode(note.nodeId, AudioUIBridge::HighlightKind::Hide, runId);
        eventManager.scheduler.removeNote(i);
    }

    eventManager.bridge.pushArrowReset(AudioUIBridge::primaryTrail(runId));
    eventManager.bridge.pushArrowReset(AudioUIBridge::modulatorTrail(runId));
}
