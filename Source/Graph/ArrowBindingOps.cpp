#include "ArrowBindingOps.h"
#include "GraphState.h"
#include "ValueTreeIdentifiers.h"

#include <algorithm>
#include <utility>

void ArrowBindingOps::setArrowInfo(juce::ValueTree arrowTree, const ArrowInfo& arrowInfo,
                                   juce::UndoManager* undoManager)
{
    if (! arrowTree.isValid()) {
        return;
    }

    arrowTree.setProperty(ValueTreeIdentifiers::ArrowType,        static_cast<int>(arrowInfo.type),     undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowXBinding,    static_cast<int>(arrowInfo.xBinding), undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowYBinding,    static_cast<int>(arrowInfo.yBinding), undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowXMultiplier, arrowInfo.xMultiplier,                undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowYMultiplier, arrowInfo.yMultiplier,                undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowSync,        arrowInfo.isSynced,                   undoManager);
    arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration,    arrowInfo.durationOverride,           undoManager);
}

ArrowInfo ArrowBindingOps::getArrowInfo(const juce::ValueTree& arrowTree)
{
    ArrowInfo arrowInfo;

    if (! arrowTree.isValid()) {
        return arrowInfo;
    }

    arrowInfo.type = static_cast<ArrowType>((int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowType,
                                                                       static_cast<int>(arrowInfo.type)));

    arrowInfo.xBinding = static_cast<ArrowBinding>((int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowXBinding,
                                                                              static_cast<int>(arrowInfo.xBinding)));
    arrowInfo.yBinding = static_cast<ArrowBinding>((int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowYBinding,
                                                                              static_cast<int>(arrowInfo.yBinding)));

    arrowInfo.xMultiplier = arrowTree.getProperty(ValueTreeIdentifiers::ArrowXMultiplier, arrowInfo.xMultiplier);
    arrowInfo.yMultiplier = arrowTree.getProperty(ValueTreeIdentifiers::ArrowYMultiplier, arrowInfo.yMultiplier);
    arrowInfo.isSynced    = arrowTree.getProperty(ValueTreeIdentifiers::ArrowSync,        arrowInfo.isSynced);

    arrowInfo.durationOverride = arrowTree.getProperty(ValueTreeIdentifiers::ArrowDuration,
                                                       arrowInfo.durationOverride);

    return arrowInfo;
}

void ArrowBindingOps::applyNodeBinding(ArrowInfo& arrowInfo, int nodeId,
                                       const juce::ValueTree& newArrow) const
{
    const juce::ValueTree node = graphState.getNode(nodeId);

    if (! node.isValid()) {
        return;
    }

    auto targetsAlternative = [this](const juce::ValueTree& arrowTree) {
        if (! arrowTree.hasProperty(ValueTreeIdentifiers::Id)) {
            return false;
        }

        const int targetId = arrowTree.getProperty(ValueTreeIdentifiers::Id);

        const juce::Identifier targetType = graphState.getNode(targetId).getType();

        return targetType == ValueTreeIdentifiers::AlternativeNodeData
            || targetType == ValueTreeIdentifiers::AlternativeModulatorData;
    };

    juce::ValueTree establishedArrow;

    const juce::ValueTree childIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren() && ! establishedArrow.isValid(); ++i) {
        if (childIds.getChild(i) != newArrow && ! targetsAlternative(childIds.getChild(i))) {
            establishedArrow = childIds.getChild(i);
        }
    }

    const juce::ValueTree danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren() && ! establishedArrow.isValid(); ++i) {
        if (danglingArrows.getChild(i) != newArrow) {
            establishedArrow = danglingArrows.getChild(i);
        }
    }

    if (establishedArrow.isValid()) {
        const ArrowInfo establishedInfo = getArrowInfo(establishedArrow);

        arrowInfo.xBinding    = establishedInfo.xBinding;
        arrowInfo.yBinding    = establishedInfo.yBinding;
        arrowInfo.xMultiplier = establishedInfo.xMultiplier;
        arrowInfo.yMultiplier = establishedInfo.yMultiplier;
    }

    if (targetsAlternative(newArrow)) {
        std::swap(arrowInfo.xBinding,    arrowInfo.yBinding);
        std::swap(arrowInfo.xMultiplier, arrowInfo.yMultiplier);

        if (arrowInfo.xBinding == ArrowBinding::PitchBind) {
            arrowInfo.xBinding = ArrowBinding::NoBind;
        }

        if (arrowInfo.yBinding == ArrowBinding::PitchBind) {
            arrowInfo.yBinding = ArrowBinding::NoBind;
        }
    }
}

bool ArrowBindingOps::applyArrowPitchOffset(juce::ValueTree arrowTree, int targetNodeId,
                                            int deltaX, int deltaY, juce::UndoManager* undoManager)
{
    const ArrowInfo arrowInfo = getArrowInfo(arrowTree);

    if (! ArrowInfo::bindsTo(arrowInfo, ArrowBinding::PitchBind)) {
        return false;
    }

    const int offset = ArrowInfo::pitchOffsetFromDelta(arrowInfo, deltaX, deltaY);

    if (! arrowTree.hasProperty(ValueTreeIdentifiers::ArrowPitchOffset)) {
        arrowTree.setProperty(ValueTreeIdentifiers::ArrowPitchOffset, offset, undoManager);
        return false;
    }

    const int appliedPitchOffset = arrowTree.getProperty(ValueTreeIdentifiers::ArrowPitchOffset, 0);

    if (offset == appliedPitchOffset) {
        return false;
    }

    juce::ValueTree note = graphState.getMidiNotes(targetNodeId)
                                     .getChildWithName(ValueTreeIdentifiers::MidiNoteData);

    if (! note.isValid()) {
        return false;
    }

    const int currentPitch = note.getProperty(ValueTreeIdentifiers::MidiPitch, defaultMidiPitch);
    const int wantedPitch  = currentPitch + offset - appliedPitchOffset;
    const int newPitch     = std::clamp(wantedPitch, minimumMidiPitch, maximumMidiPitch);

    arrowTree.setProperty(ValueTreeIdentifiers::ArrowPitchOffset, offset - (wantedPitch - newPitch), undoManager);

    if (newPitch == currentPitch) {
        return false;
    }

    note.setProperty(ValueTreeIdentifiers::MidiPitch, newPitch, undoManager);

    return true;
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

        const int durationOverride = arrowTree.getProperty(ValueTreeIdentifiers::ArrowDuration,
                                                           ArrowInfo::noDurationOverride);

        if (durationOverride != ArrowInfo::noDurationOverride) {
            arrowTree.setProperty(ValueTreeIdentifiers::ArrowDuration,
                                  ArrowInfo::noDurationOverride, undoManager);
        }
    };

    const auto parents = graphState.parentIdsOf.find(nodeId);

    if (parents != graphState.parentIdsOf.end()) {
        for (const int parentId : parents->second) {
            clearOverride(graphState.getConnection(parentId, nodeId));
        }
    }

    const juce::ValueTree childIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); ++i) {
        clearOverride(childIds.getChild(i));
    }
}

std::vector<int> ArrowBindingOps::syncPitchBindings(int nodeId, juce::UndoManager* undoManager)
{
    std::vector<int> repitchedNodeIds;

    juce::ValueTree node = graphState.getNode(nodeId);

    if (! node.isValid()) {
        return repitchedNodeIds;
    }

    auto rememberRepitched = [&repitchedNodeIds](int repitchedId) {
        if (std::find(repitchedNodeIds.begin(), repitchedNodeIds.end(), repitchedId) == repitchedNodeIds.end()) {
            repitchedNodeIds.push_back(repitchedId);
        }
    };

    const int centreX = node.getProperty(ValueTreeIdentifiers::XPosition);
    const int centreY = node.getProperty(ValueTreeIdentifiers::YPosition);

    const auto parents = graphState.parentIdsOf.find(nodeId);

    if (parents != graphState.parentIdsOf.end()) {
        for (const int parentId : parents->second) {
            const juce::ValueTree parent = graphState.getNode(parentId);

            if (! parent.isValid()) {
                continue;
            }

            const int parentX = parent.getProperty(ValueTreeIdentifiers::XPosition);
            const int parentY = parent.getProperty(ValueTreeIdentifiers::YPosition);

            if (applyArrowPitchOffset(graphState.getConnection(parentId, nodeId), nodeId,
                                      centreX - parentX, centreY - parentY, undoManager)) {
                rememberRepitched(nodeId);
            }
        }
    }

    const juce::ValueTree childIds = node.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds);

    for (int i = 0; i < childIds.getNumChildren(); ++i) {
        juce::ValueTree arrowTree = childIds.getChild(i);

        const int childId = arrowTree.getProperty(ValueTreeIdentifiers::Id);

        const juce::ValueTree child = graphState.getNode(childId);

        if (! child.isValid()) {
            continue;
        }

        const int childX = child.getProperty(ValueTreeIdentifiers::XPosition);
        const int childY = child.getProperty(ValueTreeIdentifiers::YPosition);

        if (applyArrowPitchOffset(arrowTree, childId,
                                  childX - centreX, childY - centreY, undoManager)) {
            rememberRepitched(childId);
        }
    }

    const juce::ValueTree danglingArrows = node.getChildWithName(ValueTreeIdentifiers::DanglingArrows);

    for (int i = 0; i < danglingArrows.getNumChildren(); ++i) {
        juce::ValueTree arrowTree = danglingArrows.getChild(i);

        if (applyArrowPitchOffset(arrowTree, nodeId,
                                  (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX),
                                  (int) arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY),
                                  undoManager)) {
            rememberRepitched(nodeId);
        }
    }

    return repitchedNodeIds;
}
