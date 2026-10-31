#include "AudioCommandDrainer.h"

#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/GraphState.h"
#include "../../Audio/ThreadMessaging/AudioUIBridge.h"

AudioCommandDrainer::AudioCommandDrainer(NodeCanvas& nodeCanvas, AudioUIBridge& bridge, GraphState& graphState)
    : nodeCanvas(nodeCanvas), bridge(bridge), graphState(graphState)
{
    bridge.highlights.drain([](const BridgeCommands::HighlightCommand&) {});
    bridge.arrows.drain([](const BridgeCommands::ArrowCommand&) {});

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
        bridge.highlights.drain([](const BridgeCommands::HighlightCommand&) {});
        bridge.arrows.drain([](const BridgeCommands::ArrowCommand&) {});

        nodeCanvas.clearHighlights();
        nodeCanvas.resetAllProgress();
        nodeCanvas.encapsulationView.syncHighlights();
    }
    else {
        drainHighlights();
        drainArrows();
    }

    if (droppedCounts) {
        bridge.counts.drain([](const BridgeCommands::CountCommand&) {});
    }
    else {
        drainCounts();
    }
}

void AudioCommandDrainer::drainHighlights()
{
    bridge.highlights.drain([this](const BridgeCommands::HighlightCommand& command) {
        if (command.kind == BridgeCommands::HighlightKind::ClearEveryNode) {
            nodeCanvas.clearHighlights();
            return;
        }

        Node* const node = static_cast<Node*>(nodeCanvas.nodeManager.find(command.nodeId));

        if (node == nullptr) {
            return;
        }

        const bool   shouldHighlight = command.kind == BridgeCommands::HighlightKind::Show;
        juce::Colour highlightColour = juce::Colours::white;

        if (shouldHighlight) {
            highlightColour = getTraversalColour(command.traversalId);
        }

        node->setHighlightVisual(command.runId, shouldHighlight, highlightColour);
    });

    nodeCanvas.encapsulationView.syncHighlights();
}

void AudioCommandDrainer::drainArrows()
{
    bridge.arrows.drain([this](const BridgeCommands::ArrowCommand& command) {
        if (command.kind == BridgeCommands::ArrowKind::TrailReset) {
            if (command.trailId == BridgeCommands::allTrails) {
                nodeCanvas.resetAllProgress();
                return;
            }

            nodeCanvas.resetTrail(command.trailId);
            return;
        }

        ArsNovaUI::CanvasNode* const parentNode = nodeCanvas.nodeManager.find(command.parentNodeId);

        if (parentNode == nullptr) {
            return;
        }

        const juce::Colour progressColour = getTraversalColour(command.traversalId);
        const bool         isConnection   = command.kind == BridgeCommands::ArrowKind::Connection;
        const auto         arrowRange     = parentNode->nodeArrows.equal_range(command.childNodeId);

        for (auto entry = arrowRange.first; entry != arrowRange.second; ++entry) {
            Arrow* const arrow = static_cast<Arrow*>(entry->second);

            if (arrow == nullptr) {
                continue;
            }

            const Node* const startNode = static_cast<const Node*>(arrow->startNode);
            const Node* const endNode   = static_cast<const Node*>(arrow->endNode);

            const bool startsAtFlag = startNode != nullptr && startNode->nodeType == NodeInfo::Type::TraversalFlag;
            const bool endsAtFlag   = endNode   != nullptr && endNode->nodeType   == NodeInfo::Type::TraversalFlag;

            if (startsAtFlag || endsAtFlag) {
                continue;
            }

            arrow->startProgress(command.trailId, command.durationMs, command.elapsedMs, progressColour, isConnection, command.source);
        }
    });
}

void AudioCommandDrainer::drainCounts()
{
    bridge.counts.drain([this](const BridgeCommands::CountCommand& command) {
        Node* const node = static_cast<Node*>(nodeCanvas.nodeManager.find(command.nodeId));

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
