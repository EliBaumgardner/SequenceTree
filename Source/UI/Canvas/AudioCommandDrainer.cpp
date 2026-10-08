#include "AudioCommandDrainer.h"

#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/GraphState.h"
#include "../../Audio/AudioUIBridge.h"

AudioCommandDrainer::AudioCommandDrainer(NodeCanvas& nodeCanvas, AudioUIBridge& bridge, GraphState& graphState)
    : nodeCanvas(nodeCanvas), bridge(bridge), graphState(graphState)
{
    bridge.highlights.drain([](const AudioUIBridge::HighlightCommand&) {});
    bridge.arrows.drain([](const AudioUIBridge::ArrowCommand&) {});

    bridge.highlights.overflowed.store(false);
    bridge.arrows.overflowed.store(false);
}

void AudioCommandDrainer::drainAll()
{
    const bool droppedHighlights = bridge.highlights.overflowed.exchange(false);
    const bool droppedArrows     = bridge.arrows.overflowed.exchange(false);
    const bool droppedCounts     = bridge.counts.overflowed.exchange(false);
    const bool streamBroken      = droppedHighlights || droppedArrows;

    if (streamBroken) {
        bridge.highlights.drain([](const AudioUIBridge::HighlightCommand&) {});
        bridge.arrows.drain([](const AudioUIBridge::ArrowCommand&) {});

        nodeCanvas.nodeManager.clearHighlights();
        nodeCanvas.arrowManager.resetAllProgress();
        nodeCanvas.encapsulationView.syncHighlights();
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
    bridge.highlights.drain([this](const AudioUIBridge::HighlightCommand& command) {
        if (command.kind == AudioUIBridge::HighlightKind::ClearEveryNode) {
            nodeCanvas.nodeManager.clearHighlights();
            return;
        }

        Node* const node = nodeCanvas.nodeManager.find(command.nodeId);

        if (node == nullptr) {
            return;
        }

        const bool   shouldHighlight = command.kind == AudioUIBridge::HighlightKind::Show;
        juce::Colour highlightColour = juce::Colours::white;

        if (shouldHighlight) {
            highlightColour = getTraversalColour(command.traversalId);
        }

        node->setHighlightVisual(command.runId, shouldHighlight, highlightColour);
    });

    nodeCanvas.encapsulationView.syncHighlights();
}

juce::Colour AudioCommandDrainer::getTraversalColour(int traversalId) const
{
    const juce::ValueTree traversalData = graphState.traversals.map
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

void AudioCommandDrainer::drainArrows()
{
    bridge.arrows.drain([this](const AudioUIBridge::ArrowCommand& command) {
        if (command.kind == AudioUIBridge::ArrowKind::TrailReset) {
            if (command.trailId == AudioUIBridge::allTrails) {
                nodeCanvas.arrowManager.resetAllProgress();
                return;
            }

            nodeCanvas.arrowManager.resetTrail(command.trailId);
            return;
        }

        Node* const parentNode = nodeCanvas.nodeManager.find(command.parentNodeId);

        if (parentNode == nullptr) {
            return;
        }

        const juce::Colour progressColour = getTraversalColour(command.traversalId);
        const bool         isConnection   = command.kind == AudioUIBridge::ArrowKind::Connection;
        const auto         arrowRange     = parentNode->nodeArrows.equal_range(command.childNodeId);

        for (auto entry = arrowRange.first; entry != arrowRange.second; ++entry) {
            Arrow* const arrow = entry->second;

            if (arrow == nullptr) {
                continue;
            }

            const bool startsAtFlag = arrow->startNode != nullptr && arrow->startNode->nodeType == NodeType::TraversalFlag;
            const bool endsAtFlag   = arrow->endNode   != nullptr && arrow->endNode->nodeType   == NodeType::TraversalFlag;

            if (startsAtFlag || endsAtFlag) {
                continue;
            }

            arrow->startProgress(command.trailId, command.durationMs, command.elapsedMs, progressColour, isConnection, command.source);
        }
    });
}

void AudioCommandDrainer::drainCounts()
{
    bridge.counts.drain([this](const AudioUIBridge::CountCommand& command) {
        Node* const node = nodeCanvas.nodeManager.find(command.nodeId);

        if (node == nullptr) {
            return;
        }

        node->displayCurrentCount = command.currentCount;
        node->displayCountLimit   = juce::jmax(1, command.countLimit);

        node->repaint();
    });
}
