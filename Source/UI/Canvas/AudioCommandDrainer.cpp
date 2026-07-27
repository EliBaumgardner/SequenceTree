//
// Created by Eli Baumgardner on 7/21/26.
//

#include "AudioCommandDrainer.h"

#include "NodeCanvas.h"
#include "DanglingArrowLayer.h"
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

        if (command.parentNodeId == command.childNodeId) {
            for (Arrow* const arrow : canvas.arrowManager.all()) {
                if (arrow->isDangling() && arrow->startNode == parentNode) {
                    arrow->startProgress(command.traversalId, command.durationMs, progressColour, command.isConnection);
                }
            }
            return;
        }

        const auto arrowIt = parentNode->nodeArrows.find(command.childNodeId);
        if (arrowIt == parentNode->nodeArrows.end() || arrowIt->second == nullptr) {
            return;
        }

        arrowIt->second->startProgress(command.traversalId, command.durationMs, progressColour, command.isConnection);
    });
}

void AudioCommandDrainer::drainArrowResets() const
{
    applicationContext.processor->eventManager.bridge.arrowResets.drain(
        [this](const AudioUIBridge::ResetCommand& command)
    {
        canvas.arrowManager.resetGraphProgress(command.rootId, command.traversalId);
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
