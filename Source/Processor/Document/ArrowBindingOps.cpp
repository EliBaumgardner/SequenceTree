#include "ArrowBindingOps.h"
#include "GraphState.h"
#include "TypedProperties.h"
#include "ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

#include <algorithm>

void ArrowBindingOps::syncPitchBindings(int nodeId, juce::UndoManager* undoManager)
{
    juce::ValueTree node = graphState.getNode(nodeId);

    if (! node.isValid()) {
        return;
    }

    const int centreX = node.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
    const int centreY = node.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);

    const auto parents = graphState.parentIdsOf.find(nodeId);

    if (parents != graphState.parentIdsOf.end()) {
        for (const int parentId : parents->second) {
            const juce::ValueTree parent = graphState.getNode(parentId);

            if (! parent.isValid()) {
                continue;
            }

            const int parentX = parent.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
            const int parentY = parent.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
            const int offsetX = centreX - parentX;
            const int offsetY = centreY - parentY;

            const juce::ValueTree parentArrow = graphState.getConnection(parentId, nodeId);

            applyArrowPitchOffset(parentArrow, nodeId, offsetX, offsetY, undoManager);
        }
    }

    const juce::ValueTree childIds = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); ++i) {
        juce::ValueTree arrowTree = childIds.getChild(i);

        const int childId = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id);

        const juce::ValueTree child = graphState.getNode(childId);

        if (! child.isValid()) {
            continue;
        }

        const int childX  = child.getProperty(ArsNovaUI::CanvasIdentifiers::XPosition);
        const int childY  = child.getProperty(ArsNovaUI::CanvasIdentifiers::YPosition);
        const int offsetX = childX - centreX;
        const int offsetY = childY - centreY;

        applyArrowPitchOffset(arrowTree, childId, offsetX, offsetY, undoManager);
    }

    const juce::ValueTree danglingArrows = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren(); ++i) {
        juce::ValueTree arrowTree = danglingArrows.getChild(i);

        const int tipX = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipX);
        const int tipY = arrowTree.getProperty(ArsNovaUI::CanvasIdentifiers::ArrowTipY);

        applyArrowPitchOffset(arrowTree, nodeId, tipX, tipY, undoManager);
    }
}

void ArrowBindingOps::clearArrowDurations(int nodeId, juce::UndoManager* undoManager)
{
    const juce::ValueTree node = graphState.getNode(nodeId);

    if (! node.isValid()) {
        return;
    }

    auto clearOverride = [undoManager](juce::ValueTree arrowTree) {
        if (! arrowTree.isValid()) {
            return;
        }

        ArrowProperties properties(arrowTree);

        const int durationOverride = properties.durationOverride;

        if (durationOverride != ArrowInfo::noDurationOverride) {
            properties.durationOverride.setValue(ArrowInfo::noDurationOverride, undoManager);
        }
    };

    const auto parents = graphState.parentIdsOf.find(nodeId);

    if (parents != graphState.parentIdsOf.end()) {
        for (const int parentId : parents->second) {
            clearOverride(graphState.getConnection(parentId, nodeId));
        }
    }

    const juce::ValueTree childIds = node.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); ++i) {
        clearOverride(childIds.getChild(i));
    }
}

void ArrowBindingOps::setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo::Settings& arrowInfo,
                                   juce::UndoManager* undoManager)
{
    if (! arrowTree.isValid()) {
        return;
    }

    const int typeValue     = static_cast<int>(arrowInfo.type);
    const int xBindingValue = static_cast<int>(arrowInfo.xBinding);
    const int yBindingValue = static_cast<int>(arrowInfo.yBinding);

    ArrowProperties properties(arrowTree);

    properties.arrowType       .setValue(typeValue,                  undoManager);
    properties.xBinding        .setValue(xBindingValue,              undoManager);
    properties.yBinding        .setValue(yBindingValue,              undoManager);
    properties.xMultiplier     .setValue(arrowInfo.xMultiplier,      undoManager);
    properties.yMultiplier     .setValue(arrowInfo.yMultiplier,      undoManager);
    properties.isSynced        .setValue(arrowInfo.isSynced,         undoManager);
    properties.durationOverride.setValue(arrowInfo.durationOverride, undoManager);
}

ArrowInfo::Settings ArrowBindingOps::getArrowInfo(const juce::ValueTree& arrowTree)
{
    ArrowInfo::Settings arrowInfo;

    const ArrowProperties properties(arrowTree);

    if (! arrowTree.isValid()) {
        return arrowInfo;
    }

    const int storedType     = properties.arrowType;
    const int storedXBinding = properties.xBinding;
    const int storedYBinding = properties.yBinding;

    arrowInfo.type             = static_cast<ArrowInfo::Type>(storedType);
    arrowInfo.xBinding         = static_cast<ArrowInfo::Binding>(storedXBinding);
    arrowInfo.yBinding         = static_cast<ArrowInfo::Binding>(storedYBinding);
    arrowInfo.xMultiplier      = properties.xMultiplier;
    arrowInfo.yMultiplier      = properties.yMultiplier;
    arrowInfo.isSynced         = properties.isSynced;
    arrowInfo.durationOverride = properties.durationOverride;

    return arrowInfo;
}

void ArrowBindingOps::applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId,
                                            int deltaX, int deltaY, juce::UndoManager* undoManager)
{
    const ArrowInfo::Settings arrowInfo = getArrowInfo(arrowTree);

    if (! ArrowInfo::Settings::bindsTo(arrowInfo, ArrowInfo::Binding::PitchBind)) {
        return;
    }

    const int offset = ArrowInfo::Settings::pitchOffsetFromDelta(arrowInfo, deltaX, deltaY);

    ArrowProperties properties(arrowTree);

    if (properties.pitchOffset.isUsingDefault()) {
        properties.pitchOffset.setValue(offset, undoManager);

        return;
    }

    const int appliedPitchOffset = properties.pitchOffset;

    if (offset == appliedPitchOffset) {
        return;
    }

    juce::ValueTree note = graphState.getMidiNotes(targetNodeId)
                                     .getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    if (! note.isValid()) {
        return;
    }

    NoteProperties noteProperties(note);

    int currentPitch = noteProperties.midiPitch;

    if (noteProperties.midiPitch.isUsingDefault()) {
        currentPitch = NodeInfo::defaultMidiPitch;
    }

    const int wantedPitch = currentPitch + offset - appliedPitchOffset;
    const int newPitch    = std::clamp(wantedPitch, NodeInfo::minimumMidiPitch, NodeInfo::maximumMidiPitch);
    const int keptOffset  = offset - (wantedPitch - newPitch);

    properties.pitchOffset  .setValue(keptOffset, undoManager);
    noteProperties.midiPitch.setValue(newPitch,   undoManager);
}
