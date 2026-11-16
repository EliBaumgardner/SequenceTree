#include "TypedProperties.h"
#include "GraphState.h"
#include "ValueTreeIdentifiers.h"
#include "../Util/ArrowInfo.h"

NodeProperties::NodeProperties(juce::ValueTree node)
{
    rootNodeId        .referTo(node, ValueTreeIdentifiers::RootNodeId,        nullptr);
    encapsulatorId    .referTo(node, ValueTreeIdentifiers::EncapsulatorId,    nullptr, noEncapsulatorId);
    encapsulatorLabel .referTo(node, ValueTreeIdentifiers::EncapsulatorLabel, nullptr);
    countLimit        .referTo(node, ValueTreeIdentifiers::CountLimit,        nullptr);
    triggerLimit      .referTo(node, ValueTreeIdentifiers::TriggerLimit,      nullptr, GraphState::defaultTriggerLimit);
    loopLimit         .referTo(node, ValueTreeIdentifiers::LoopLimit,         nullptr, GraphState::defaultRootLoopLimit);
    switchCountLimit  .referTo(node, ValueTreeIdentifiers::SwitchCountLimit,  nullptr);
    subLoopCountLimit .referTo(node, ValueTreeIdentifiers::SubLoopCountLimit, nullptr);
    repeatValue       .referTo(node, ValueTreeIdentifiers::RepeatValue,       nullptr, GraphState::defaultRepeatValue);
    probability       .referTo(node, ValueTreeIdentifiers::Probability,       nullptr, GraphState::defaultProbability);
    modAmount         .referTo(node, ValueTreeIdentifiers::ModAmount,         nullptr, GraphState::defaultModAmount);
    radius            .referTo(node, ValueTreeIdentifiers::Radius,            nullptr);
    traversalFlagValue.referTo(node, ValueTreeIdentifiers::TraversalFlagValue, nullptr);
    traversalInstance .referTo(node, ValueTreeIdentifiers::TraversalInstance, nullptr);
    nodeColour        .referTo(node, ValueTreeIdentifiers::NodeColour,        nullptr);
    ornamentUid       .referTo(node, ValueTreeIdentifiers::OrnamentUid,       nullptr);
}

NoteProperties::NoteProperties(juce::ValueTree note)
{
    midiPitch   .referTo(note, ValueTreeIdentifiers::MidiPitch,    nullptr);
    midiVelocity.referTo(note, ValueTreeIdentifiers::MidiVelocity, nullptr);
    midiDuration.referTo(note, ValueTreeIdentifiers::MidiDuration, nullptr);
    midiChannel .referTo(note, ValueTreeIdentifiers::MidiChannel,  nullptr, GraphState::defaultMidiChannel);
}

ArrowProperties::ArrowProperties(juce::ValueTree arrow)
{
    const ArrowInfo::Settings defaults;

    const int defaultType     = static_cast<int>(defaults.type);
    const int defaultXBinding = static_cast<int>(defaults.xBinding);
    const int defaultYBinding = static_cast<int>(defaults.yBinding);

    arrowType       .referTo(arrow, ValueTreeIdentifiers::ArrowType,        nullptr, defaultType);
    xBinding        .referTo(arrow, ValueTreeIdentifiers::ArrowXBinding,    nullptr, defaultXBinding);
    yBinding        .referTo(arrow, ValueTreeIdentifiers::ArrowYBinding,    nullptr, defaultYBinding);
    xMultiplier     .referTo(arrow, ValueTreeIdentifiers::ArrowXMultiplier, nullptr, defaults.xMultiplier);
    yMultiplier     .referTo(arrow, ValueTreeIdentifiers::ArrowYMultiplier, nullptr, defaults.yMultiplier);
    isSynced        .referTo(arrow, ValueTreeIdentifiers::ArrowSync,        nullptr, defaults.isSynced);
    durationOverride.referTo(arrow, ValueTreeIdentifiers::ArrowDuration,    nullptr, defaults.durationOverride);
    pitchOffset     .referTo(arrow, ValueTreeIdentifiers::ArrowPitchOffset, nullptr);
    countLimit      .referTo(arrow, ValueTreeIdentifiers::CountLimit,       nullptr, GraphState::defaultNodeCountLimit);
}

TraversalProperties::TraversalProperties(juce::ValueTree traversal)
{
    traversalId    .referTo(traversal, ValueTreeIdentifiers::TraversalId,        nullptr);
    tempoMultiplier.referTo(traversal, ValueTreeIdentifiers::TempoMultiplier,    nullptr);
    colour         .referTo(traversal, ValueTreeIdentifiers::TraversalColour,    nullptr);
    channel        .referTo(traversal, ValueTreeIdentifiers::TraversalChannel,   nullptr);
    transpose      .referTo(traversal, ValueTreeIdentifiers::TraversalTranspose, nullptr);
    velocity       .referTo(traversal, ValueTreeIdentifiers::TraversalVelocity,  nullptr);
}

TraversalReferenceProperties::TraversalReferenceProperties(juce::ValueTree reference)
{
    traversalId      .referTo(reference, ValueTreeIdentifiers::TraversalId,       nullptr);
    traversalInstance.referTo(reference, ValueTreeIdentifiers::TraversalInstance, nullptr);
}

RuleProperties::RuleProperties(juce::ValueTree rule)
{
    ruleName  .referTo(rule, ValueTreeIdentifiers::RuleName,     nullptr);
    ruleSource.referTo(rule, ValueTreeIdentifiers::RuleSource,   nullptr);
    folderId  .referTo(rule, ValueTreeIdentifiers::RuleFolderId, nullptr);
}

FolderProperties::FolderProperties(juce::ValueTree folder)
{
    folderId      .referTo(folder, ValueTreeIdentifiers::FolderId,       nullptr);
    folderName    .referTo(folder, ValueTreeIdentifiers::FolderName,     nullptr);
    parentFolderId.referTo(folder, ValueTreeIdentifiers::FolderParentId, nullptr);
}

LibraryProperties::LibraryProperties(juce::ValueTree library)
{
    libraryUid .referTo(library, ValueTreeIdentifiers::LibraryUid,  nullptr);
    libraryName.referTo(library, ValueTreeIdentifiers::LibraryName, nullptr);
}

OrnamentProperties::OrnamentProperties(juce::ValueTree ornament)
{
    ornamentUid     .referTo(ornament, ValueTreeIdentifiers::OrnamentUid,      nullptr);
    ornamentName    .referTo(ornament, ValueTreeIdentifiers::OrnamentName,     nullptr);
    symbol          .referTo(ornament, ValueTreeIdentifiers::OrnamentSymbol,   nullptr);
    nextFigureNodeId.referTo(ornament, ValueTreeIdentifiers::NextFigureNodeId, nullptr, firstFigureNodeId);
    listIndex       .referTo(ornament, ValueTreeIdentifiers::OrnamentIndex,    nullptr, unlistedIndex);
}

FigureNoteProperties::FigureNoteProperties(juce::ValueTree figureNote)
{
    offset  .referTo(figureNote, ValueTreeIdentifiers::OrnamentOffset,   nullptr);
    velocity.referTo(figureNote, ValueTreeIdentifiers::OrnamentVelocity, nullptr);
}

PatchProperties::PatchProperties(juce::ValueTree patch)
{
    patchName.referTo(patch, ValueTreeIdentifiers::PatchName, nullptr);
    patchFile.referTo(patch, ValueTreeIdentifiers::PatchFile, nullptr);
}

CanvasSettingsProperties::CanvasSettingsProperties(juce::ValueTree canvasSettings)
{
    const ArrowInfo::Settings defaults;

    const int defaultXBinding = static_cast<int>(defaults.xBinding);
    const int defaultYBinding = static_cast<int>(defaults.yBinding);

    graphGridMultiplier   .referTo(canvasSettings, ValueTreeIdentifiers::GraphGridMultiplier,    nullptr);
    ornamentGridMultiplier.referTo(canvasSettings, ValueTreeIdentifiers::OrnamentGridMultiplier, nullptr);
    arrowXBinding         .referTo(canvasSettings, ValueTreeIdentifiers::ArrowXBinding,          nullptr, defaultXBinding);
    arrowYBinding         .referTo(canvasSettings, ValueTreeIdentifiers::ArrowYBinding,          nullptr, defaultYBinding);
    arrowXMultiplier      .referTo(canvasSettings, ValueTreeIdentifiers::ArrowXMultiplier,       nullptr, defaults.xMultiplier);
    arrowYMultiplier      .referTo(canvasSettings, ValueTreeIdentifiers::ArrowYMultiplier,       nullptr, defaults.yMultiplier);
    arrowXTarget          .referTo(canvasSettings, ValueTreeIdentifiers::ArrowXTarget,           nullptr, defaultXBinding);
    arrowYTarget          .referTo(canvasSettings, ValueTreeIdentifiers::ArrowYTarget,           nullptr, defaultYBinding);
}
