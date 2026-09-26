//
// Created by Eli Baumgardner on 7/21/26.
//

#include "AudioCommandDrainer.h"

#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/GraphState.h"
#include "../../Plugin/PluginProcessor.h"
#include "../../Util/ApplicationContext.h"

AudioCommandDrainer::AudioCommandDrainer(NodeCanvas& canvasRef, const ApplicationContext& context)
    : canvas(canvasRef), applicationContext(context)
{
    AudioUIBridge& bridge = applicationContext.processor->eventManager.bridge;

    bridge.highlights.drain([](const AudioUIBridge::HighlightCommand&) {});
    bridge.arrows    .drain([](const AudioUIBridge::ArrowCommand&)     {});

    bridge.highlights.overflowed.store(false);
    bridge.arrows    .overflowed.store(false);
}

void AudioCommandDrainer::drainAll()
{
    AudioUIBridge& bridge = applicationContext.processor->eventManager.bridge;

    const bool droppedHighlights = bridge.highlights.overflowed.exchange(false);
    const bool droppedArrows     = bridge.arrows.overflowed.exchange(false);
    const bool droppedCounts     = bridge.counts.overflowed.exchange(false);

    const bool streamBroken = droppedHighlights || droppedArrows;

    if (streamBroken) {
        bridge.highlights.drain([](const AudioUIBridge::HighlightCommand&) {});
        bridge.arrows    .drain([](const AudioUIBridge::ArrowCommand&)     {});

        canvas.nodeManager.clearHighlights();
        canvas.arrowManager.resetAllProgress();
        canvas.encapsulationView.syncHighlights();
    }
    else {
        drainHighlights();
        drainArrows();
    }

    if (droppedCounts) {
        bridge.counts.drain([](const AudioUIBridge::CountCommand&) {});
    }
    else {
        drainCounts();
    }
}

void AudioCommandDrainer::drainHighlights()
{
    applicationContext.processor->eventManager.bridge.highlights.drain(
        [this](const AudioUIBridge::HighlightCommand& command)
    {
        if (command.kind == AudioUIBridge::HighlightKind::ClearEveryNode) {
            canvas.nodeManager.clearHighlights();
            return;
        }

        Node* const node = canvas.nodeManager.find(command.nodeId);
        if (node == nullptr) {
            return;
        }

        const bool shouldHighlight = command.kind == AudioUIBridge::HighlightKind::Show;

        juce::Colour highlightColour = juce::Colours::white;

        if (shouldHighlight) {
            highlightColour = getTraversalColour(command.traversalId);
        }

        node->setHighlightVisual(command.runId, shouldHighlight, highlightColour);
    });

    canvas.encapsulationView.syncHighlights();
}

void AudioCommandDrainer::drainArrows()
{
    applicationContext.processor->eventManager.bridge.arrows.drain(
        [this](const AudioUIBridge::ArrowCommand& command)
    {
        if (command.kind == AudioUIBridge::ArrowKind::TrailReset) {
            if (command.trailId == AudioUIBridge::allTrails) {
                canvas.arrowManager.resetAllProgress();
                return;
            }

            canvas.arrowManager.resetTrail(command.trailId);
            return;
        }

        Node* const parentNode = canvas.nodeManager.find(command.parentNodeId);
        if (parentNode == nullptr) {
            return;
        }

        const juce::Colour progressColour = getTraversalColour(command.traversalId);

        const bool isConnection = command.kind == AudioUIBridge::ArrowKind::Connection;

        const auto range = parentNode->nodeArrows.equal_range(command.childNodeId);

        for (auto entry = range.first; entry != range.second; ++entry) {
            if (entry->second == nullptr || ((entry->second->startNode != nullptr && entry->second->startNode->nodeType == NodeType::TraversalFlag)
        || (entry->second->endNode   != nullptr && entry->second->endNode->nodeType   == NodeType::TraversalFlag))) {
                continue;
            }

            entry->second->startProgress(command.trailId, command.durationMs, command.elapsedMs,
                                         progressColour, isConnection);
        }
    });
}

void AudioCommandDrainer::drainCounts()
{
    applicationContext.processor->eventManager.bridge.counts.drain(
        [this](const AudioUIBridge::CountCommand& command)
    {
        Node* const node = canvas.nodeManager.find(command.nodeId);
        if (node == nullptr) {
            return;
        }

        node->displayCurrentCount = command.currentCount;
        node->displayCountLimit   = juce::jmax(1, command.countLimit);
        node->repaint();
    });
}

juce::Colour AudioCommandDrainer::getTraversalColour(int traversalId) const
{
    const juce::ValueTree traversalData = applicationContext.graphState->traversals.map
        .getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

    if (!traversalData.isValid()) {
        return juce::Colours::white;
    }

    const juce::String colourString = traversalData.getProperty(ValueTreeIdentifiers::TraversalColour).toString();

    if (colourString.isEmpty()) {
        return juce::Colours::white;
    }

    return juce::Colour::fromString(colourString);
}
