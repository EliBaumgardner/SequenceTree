//
// Created by Eli Baumgardner on 7/21/26.
//

#include "AudioCommandDrainer.h"

#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Plugin/PluginProcessor.h"
#include "../../Util/ApplicationContext.h"

AudioCommandDrainer::AudioCommandDrainer(NodeCanvas& canvasRef, ApplicationContext& context)
    : canvas(canvasRef), applicationContext(context)
{
}

void AudioCommandDrainer::drainAll() const
{
    drainHighlights();
    drainArrowResets();
    drainProgress();
    drainCounts();
}

juce::Colour AudioCommandDrainer::getTraversalColour(int traversalId) const
{
    const juce::ValueTree traversalData = applicationContext.valueTreeState->traversalMap
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

void AudioCommandDrainer::drainHighlights() const
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
            highlightColour = getTraversalColour(command.traversalId);
        }

        node->setHighlightVisual(command.traversalId, command.shouldHighlight, highlightColour);
    });
}

void AudioCommandDrainer::drainProgress() const
{
    applicationContext.processor->eventManager.bridge.progress.drain(
        [this](const AudioUIBridge::ProgressCommand& command)
    {
        Node* const parentNode = canvas.nodeManager.find(command.parentNodeId);
        if (parentNode == nullptr) {
            return;
        }

        const juce::Colour progressColour = getTraversalColour(command.traversalId);

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

void AudioCommandDrainer::drainArrowResets() const
{
    applicationContext.processor->eventManager.bridge.arrowResets.drain(
        [this](const AudioUIBridge::ResetCommand& command)
    {
        if (command.trailId == AudioUIBridge::allTrails) {
            canvas.arrowManager.resetAllProgress();
            return;
        }

        canvas.arrowManager.resetTrail(command.trailId);
    });
}

void AudioCommandDrainer::drainCounts() const
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
