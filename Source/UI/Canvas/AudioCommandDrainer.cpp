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

AudioCommandDrainer::AudioCommandDrainer(NodeCanvas& canvasRef, ApplicationContext& context)
    : canvas(canvasRef), applicationContext(context)
{
}

void AudioCommandDrainer::drainAll()
{
    AudioUIBridge& bridge = applicationContext.processor->eventManager.bridge;

    const bool droppedHighlights = bridge.highlights.overflowed.exchange(false);
    const bool droppedArrows     = bridge.arrows.overflowed.exchange(false);

    const bool streamBroken = needsResync || droppedHighlights || droppedArrows;

    if (streamBroken) {
        bridge.highlights.drain([](const AudioUIBridge::HighlightCommand&) {});
        bridge.arrows    .drain([](const AudioUIBridge::ArrowCommand&)     {});

        canvas.nodeManager.clearHighlights();
        canvas.arrowManager.resetAllProgress();
        canvas.encapsulationView.syncHighlights();

        needsResync = false;
    }
    else {
        drainHighlights();
        drainArrows();
    }

    drainCounts();
}

juce::Colour AudioCommandDrainer::getTraversalColour(int typeId) const
{
    const juce::ValueTree traversalData = applicationContext.graphState->traversals.map
        .getChildWithProperty(ValueTreeIdentifiers::TraversalId, typeId);

    if (!traversalData.isValid()) {
        return juce::Colours::white;
    }

    const juce::String colourString = traversalData.getProperty(ValueTreeIdentifiers::TraversalColour).toString();

    if (colourString.isEmpty()) {
        return juce::Colours::white;
    }

    return juce::Colour::fromString(colourString);
}

void AudioCommandDrainer::drainHighlights()
{
    applicationContext.processor->eventManager.bridge.highlights.drain(
        [this](const AudioUIBridge::HighlightCommand& command)
    {
        if (command.nodeId == AudioUIBridge::allNodes) {
            canvas.nodeManager.clearHighlights();
            return;
        }

        Node* const node = canvas.nodeManager.find(command.nodeId);
        if (node == nullptr) {
            return;
        }

        juce::Colour highlightColour = juce::Colours::white;

        if (command.shouldHighlight) {
            highlightColour = getTraversalColour(command.typeId);
        }

        node->setHighlightVisual(command.runId, command.shouldHighlight, highlightColour);
    });

    canvas.encapsulationView.syncHighlights();
}

void AudioCommandDrainer::drainArrows()
{
    applicationContext.processor->eventManager.bridge.arrows.drain(
        [this](const AudioUIBridge::ArrowCommand& command)
    {
        if (command.isReset) {
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

        const juce::Colour progressColour = getTraversalColour(command.typeId);

        const auto range = parentNode->nodeArrows.equal_range(command.childNodeId);

        for (auto entry = range.first; entry != range.second; ++entry) {
            if (entry->second == nullptr || entry->second->connectsTraversalFlag()) {
                continue;
            }

            entry->second->startProgress(command.trailId, command.durationMs,
                                         progressColour, command.isConnection);
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
