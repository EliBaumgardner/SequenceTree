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

void AudioCommandDrainer::drainAll()
{
    drainHighlights();
    drainArrowResets();
    drainProgress();
    drainCounts();
}

juce::Colour AudioCommandDrainer::getTraversalColour(int traversalId) const
{
    juce::ValueTree traversalData = applicationContext.valueTreeState->traversalMap
        .getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

    if (!traversalData.isValid()) {
        return juce::Colours::white;
    }

    juce::String colourString = traversalData.getProperty(ValueTreeIdentifiers::TraversalColour).toString();

    if (colourString.isEmpty()) {
        return juce::Colours::white;
    }

    return juce::Colour::fromString(colourString);
}

void AudioCommandDrainer::drainHighlights()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.highlightFifo.read(bridge.highlightFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::HighlightCommand& command)
    {
        Node* node = canvas.nodeManager.find(command.nodeId);
        if (node == nullptr) {
            return;
        }

        const juce::Colour highlightColour = command.shouldHighlight
            ? getTraversalColour(command.traversalId)
            : juce::Colours::white;

        node->setHighlightVisual(command.traversalId, command.shouldHighlight, highlightColour);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.highlightBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.highlightBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void AudioCommandDrainer::drainProgress()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.progressFifo.read(bridge.progressFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::ProgressCommand& command)
    {
        Node* parentNode = canvas.nodeManager.find(command.parentNodeId);
        if (parentNode == nullptr) {
            return;
        }

        const juce::Colour progressColour = getTraversalColour(command.traversalId);

        if (command.parentNodeId == command.childNodeId) {
            for (Arrow* arrow : canvas.arrowManager.all()) {
                if (arrow->isDangling() && arrow->startNode == parentNode) {
                    arrow->startProgress(command.traversalId, command.durationMs, progressColour, command.isConnection);
                }
            }
            return;
        }

        auto arrowIt = parentNode->nodeArrows.find(command.childNodeId);
        if (arrowIt == parentNode->nodeArrows.end() || arrowIt->second == nullptr) {
            return;
        }

        arrowIt->second->startProgress(command.traversalId, command.durationMs, progressColour, command.isConnection);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.progressBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void AudioCommandDrainer::drainArrowResets()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.arrowResetFifo.read(bridge.arrowResetFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::ResetCommand& command)
    {
        canvas.arrowManager.resetGraphProgress(command.rootId, command.traversalId);
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.arrowResetBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.arrowResetBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}

void AudioCommandDrainer::drainCounts()
{
    auto& bridge = applicationContext.processor->eventManager.bridge;
    const auto scope = bridge.countFifo.read(bridge.countFifo.getNumReady());

    auto apply = [this](const AudioUIBridge::CountCommand& command)
    {
        Node* node = canvas.nodeManager.find(command.nodeId);
        if (node == nullptr) {
            return;
        }

        node->displayCurrentCount = command.currentCount;
        node->displayCountLimit   = juce::jmax(1, command.countLimit);
        node->repaint();
    };

    for (int i = 0; i < scope.blockSize1; ++i)
        apply(bridge.countBuffer[static_cast<size_t>(scope.startIndex1 + i)]);
    for (int i = 0; i < scope.blockSize2; ++i)
        apply(bridge.countBuffer[static_cast<size_t>(scope.startIndex2 + i)]);
}
