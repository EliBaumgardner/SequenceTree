#pragma once

#include <juce_data_structures/juce_data_structures.h>

/*
    The names used in the saved document. Each static member is a juce::Identifier used as a
    ValueTree type (NodeData, TraversalData, OrnamentLibrary) or a property name (RootNodeId,
    Radius, MidiPitch) in the graph, traversal, rule, ornament and patch trees. Code uses these
    members instead of string literals, so each name is spelled in one place. The names the canvas
    classes read - a node's Id, XPosition and YPosition, its NodeChildrenIds and its dangling arrows
    - are ArsNovaUI::CanvasIdentifiers.

    The strings are written into every saved plugin state and ornament file, so changing one makes
    older saves load without that value.
*/

class ValueTreeIdentifiers {

public:

    static const juce::Identifier PluginState;
    static const juce::Identifier NodeMap;
    static const juce::Identifier Patch;
    static const juce::Identifier PatchName;
    static const juce::Identifier PatchFile;

    static const juce::Identifier RootNodeData;
    static const juce::Identifier NodeData;
    static const juce::Identifier AlternativeNodeData;
    static const juce::Identifier TraversalFlagData;
    static const juce::Identifier ModulatorData;
    static const juce::Identifier ModulatorRootData;
    static const juce::Identifier AlternativeModulatorData;
    static const juce::Identifier EncapsulatorData;

    static const juce::Identifier EncapsulatedIds;
    static const juce::Identifier EncapsulatorId;
    static const juce::Identifier EncapsulatorLabel;

    static const juce::Identifier SelectionClipboard;

    static const juce::Identifier MidiNotesData;
    static const juce::Identifier MidiNoteData;

    static const juce::Identifier ArrowType;
    static const juce::Identifier ArrowDuration;
    static const juce::Identifier ArrowSync;

    static const juce::Identifier ArrowXBinding;
    static const juce::Identifier ArrowYBinding;
    static const juce::Identifier ArrowXMultiplier;
    static const juce::Identifier ArrowYMultiplier;
    static const juce::Identifier ArrowPitchOffset;

    static const juce::Identifier RootNodeId;
    static const juce::Identifier NodeId;

    static const juce::Identifier CountLimit;
    static const juce::Identifier TriggerLimit;
    static const juce::Identifier LoopLimit;
    static const juce::Identifier SwitchCountLimit;
    static const juce::Identifier SubLoopCountLimit;

    static const juce::Identifier RepeatValue;
    static const juce::Identifier Probability;

    static const juce::Identifier Radius;
    static const juce::Identifier NodeColour;

    static const juce::Identifier MidiPitch;
    static const juce::Identifier MidiVelocity;
    static const juce::Identifier MidiDuration;
    static const juce::Identifier MidiChannel;

    static const juce::Identifier ModAmount;

    static const juce::Identifier TraversalData;
    static const juce::Identifier TraversalMap;

    static const juce::Identifier TraversalId;
    static const juce::Identifier TraversalInstance;
    static const juce::Identifier TraversalChildrenIds;
    static const juce::Identifier DisabledTraversalIds;
    static const juce::Identifier TraversalFlagValue;

    static const juce::Identifier TraversalRules;
    static const juce::Identifier TraversalRuleData;

    static const juce::Identifier RuleName;
    static const juce::Identifier RuleSource;
    static const juce::Identifier ActiveRuleId;

    static const juce::Identifier TempoMultiplier;
    static const juce::Identifier TraversalColour;
    static const juce::Identifier TraversalChannel;
    static const juce::Identifier TraversalTranspose;
    static const juce::Identifier TraversalVelocity;

    static const juce::Identifier OrnamentLibraries;
    static const juce::Identifier OrnamentLibrary;
    static const juce::Identifier LibraryUid;
    static const juce::Identifier LibraryName;

    static const juce::Identifier OrnamentData;
    static const juce::Identifier OrnamentFigure;
    static const juce::Identifier PublishedFigure;
    static const juce::Identifier OrnamentNoteData;

    static const juce::Identifier OrnamentUid;
    static const juce::Identifier OrnamentName;
    static const juce::Identifier OrnamentSymbol;
    static const juce::Identifier OrnamentHostId;
    static const juce::Identifier NextFigureNodeId;

    static const juce::Identifier OrnamentOffset;
    static const juce::Identifier OrnamentVelocity;

    static const juce::Identifier ActiveOrnamentUid;
    static const juce::Identifier OrnamentIndex;
    static const juce::Identifier LegacyLibrariesImported;
};