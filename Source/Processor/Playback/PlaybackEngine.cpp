#include "PlaybackEngine.h"
#include "Core/Model/NodeInfo.h"

#include <cmath>

PlaybackEngine::PlaybackEngine(AudioUIBridge& bridge)
    : eventManager       (bridge),
      previewEventManager(bridge),
      traversalSession   (eventManager, previewEventManager)
{
}

void PlaybackEngine::prepare(double sampleRate)
{
    tempoInfo.currentSampleRate = sampleRate;

    traversalSession.prepare();
}

void PlaybackEngine::processBlock(const Snapshot* snap, juce::MidiBuffer& midiMessages, const BlockInfo& block) noexcept
{
    bool resetHit = resetRequested.exchange(false);

    if (resetHit && traversalSession.playback == TraversalSession::Playback::Replaying) {
        tempoInfo.expectedPpq.reset();

        resetHit = false;
    }

    if (block.followsHost) {
        followHostTransport(block);
    }

    const bool playing         = isPlaying.load();
    const bool suspended       = wasPlaying && !playing;
    const bool hasGraph        = snap != nullptr && snap->globalNodes != nullptr;
    const bool replayWanted    = tempoInfo.pendingRelocationSamples.has_value() || traversalSession.playback == TraversalSession::Playback::Replaying;
    const bool replayThisBlock = replayWanted && !resetHit;

    const RTScript* activeScript = nullptr;

    if (snap != nullptr) {
        activeScript = snap->traversalScript.get();
    }

    for (const auto& noteOff : pendingNoteOffs) {
        midiMessages.addEvent(noteOff, 0);
    }

    pendingNoteOffs.clear();

    if (resetHit) {
        traversalSession.silenceAllNotes(midiMessages);
    }

    if (suspended) {
        traversalSession.suspendActiveNotes(midiMessages);
    }

    wasPlaying = playing;

    traversalSession.setTraversalScript(activeScript);

    if (hasGraph) {
        const WalkRequest request { .playing = playing, .resetHit = resetHit, .replayThisBlock = replayThisBlock };

        driveWalk(*snap, midiMessages, block, request);
    }
    else if (resetHit) {
        traversalSession.clearTraversals();
    }
}

void PlaybackEngine::releaseNotes()
{
    pendingNoteOffs.clear();

    for (EventManager* manager : { &eventManager, &previewEventManager }) {
        for (const auto& note : manager->scheduler.activeNotes) {
            if (NoteScheduler::isNoteSounding(note)) {
                pendingNoteOffs.push_back(juce::MidiMessage::noteOff(note.event.midiChannel, note.event.pitch));
            }
        }

        manager->scheduler.clear();
    }
}

void PlaybackEngine::followHostTransport(const BlockInfo& block) noexcept
{
    bool hostPlaying = false;

    if (block.playHead != nullptr) {
        if (const juce::Optional<juce::AudioPlayHead::PositionInfo> position = block.playHead->getPosition()) {
            hostPlaying = position->getIsPlaying();

            if (const juce::Optional<double> hostBpm = position->getBpm()) {
                tempoInfo.hostBpm = *hostBpm;
            }

            const juce::Optional<double> hostPpq = position->getPpqPosition();

            if (hostPpq && tempoInfo.hostBpm > 0.0) {
                const bool resumed           = hostPlaying && !tempoInfo.hostWasPlaying;
                const bool resumeNeedsReplay = resumed && traversalSession.playback == TraversalSession::Playback::Live;
                const bool moved             = tempoInfo.hostPositionKnown && (!tempoInfo.expectedPpq || std::abs(*hostPpq - *tempoInfo.expectedPpq) > TempoInfo::relocationToleranceBeats);
                const bool jumped            = resumeNeedsReplay || moved;

                if (jumped && block.syncedToHost) {
                    const double hostBeats   = juce::jmax(0.0, *hostPpq);
                    const double hostSeconds = hostBeats * TempoInfo::secondsPerMinute / tempoInfo.hostBpm;

                    tempoInfo.pendingRelocationSamples = std::round(hostSeconds * tempoInfo.currentSampleRate);
                }

                tempoInfo.expectedPpq       = *hostPpq;
                tempoInfo.hostPositionKnown = true;

                if (hostPlaying) {
                    tempoInfo.expectedPpq = *hostPpq + block.numSamples / tempoInfo.currentSampleRate * tempoInfo.hostBpm / TempoInfo::secondsPerMinute;
                }
            }
            else {
                tempoInfo.expectedPpq.reset();
            }
        }
    }

    tempoInfo.hostWasPlaying = hostPlaying;

    if (hostPlaying != isPlaying.exchange(hostPlaying)) {
        playbackStateChanged.store(true);
    }
}

void PlaybackEngine::driveWalk(const Snapshot& snap, juce::MidiBuffer& midiMessages, const BlockInfo& block, const WalkRequest& request) noexcept
{
    double hostTempoScale = 1.0;

    if (tempoInfo.hostBpm > 0.0) {
        hostTempoScale = tempoInfo.hostBpm / TempoInfo::referenceBpm;
    }

    const DispatchContext context {
        *snap.globalNodes,
        traversalSession.traversals,
        midiMessages,
        tempoInfo.currentSampleRate,
        block.tempoParameter * hostTempoScale,
        block.velocityParameter / static_cast<double>(NodeInfo::maximumMidiVelocity),
        block.transpose
    };

    const DispatchContext previewContext { context.nodes, traversalSession.previewTraversals, context.midiMessages,
                                           context.sampleRate, context.tempoMultiplier,
                                           context.velocityScale, context.transpose };

    traversalSession.playPreview(previewContext, block.numSamples);

    if (!request.playing && !request.replayThisBlock) {
        if (request.resetHit) {
            traversalSession.clearTraversals();
        }

        return;
    }

    eventManager.followTempo(context.tempoMultiplier);

    if (tempoInfo.pendingRelocationSamples) {
        traversalSession.beginReplay(context, *tempoInfo.pendingRelocationSamples);

        tempoInfo.pendingRelocationSamples.reset();
    }

    traversalSession.renderingOffline = block.offline;

    if (traversalSession.continueReplay(context, snap.generation, block.numSamples, request.playing) == TraversalSession::Playback::Replaying) {
        return;
    }

    if (!request.playing) {
        return;
    }

    if (traversalSession.playback == TraversalSession::Playback::Suspended) {
        traversalSession.resumeSuspendedNotes(midiMessages);
    }

    if (request.resetHit) {
        traversalSession.restartActiveTraversals(context);
    }

    traversalSession.syncWithGraph(context, snap.generation);

    if (traversalSession.traversals.empty() && !traversalSession.startTraversalsFromFirstRoot(context)) {
        if (!eventManager.scheduler.activeNotes.empty()) {
            traversalSession.silenceAllNotes(midiMessages);
        }

        return;
    }

    eventManager.processEvents(block.numSamples, context);
}
