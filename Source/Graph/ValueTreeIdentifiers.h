#ifndef SEQUENCETREE_VALUETREEIDENTIFIERS_H
#define SEQUENCETREE_VALUETREEIDENTIFIERS_H

#include <juce_data_structures/juce_data_structures.h>

class ValueTreeIdentifiers {

public:

    static const juce::Identifier PluginState;
    static const juce::Identifier NodeMap;

    static const juce::Identifier RootNodeData;
    static const juce::Identifier NodeData;
    static const juce::Identifier AlternativeNodeData;
    static const juce::Identifier TraversalFlagData;
    static const juce::Identifier ModulatorData;
    static const juce::Identifier ModulatorRootData;
    static const juce::Identifier EncapsulatorData;

    static const juce::Identifier NodeChildrenIds;
    static const juce::Identifier EncapsulatedIds;
    static const juce::Identifier EncapsulatorId;
    static const juce::Identifier EncapsulatorLabel;

    static const juce::Identifier SelectionClipboard;

    static const juce::Identifier MidiNotesData;
    static const juce::Identifier MidiNoteData;

    static const juce::Identifier DanglingArrows;
    static const juce::Identifier DanglingArrow;
    static const juce::Identifier ArrowTipX;
    static const juce::Identifier ArrowTipY;
    static const juce::Identifier ArrowType;
    static const juce::Identifier ArrowXBinding;
    static const juce::Identifier ArrowYBinding;
    static const juce::Identifier ArrowXMultiplier;
    static const juce::Identifier ArrowYMultiplier;
    static const juce::Identifier ArrowPitchOffset;
    static const juce::Identifier ArrowDuration;
    static const juce::Identifier ArrowSync;

    static const juce::Identifier Id;

    static const juce::Identifier RootNodeId;
    static const juce::Identifier NodeId;

    static const juce::Identifier CountLimit;
    static const juce::Identifier TriggerLimit;
    static const juce::Identifier LoopLimit;
    static const juce::Identifier SwitchCountLimit;
    static const juce::Identifier SubLoopCountLimit;

    static const juce::Identifier RepeatValue;
    static const juce::Identifier Probability;

    static const juce::Identifier XPosition;
    static const juce::Identifier YPosition;
    static const juce::Identifier Radius;

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
};

#endif //SEQUENCETREE_VALUETREEIDENTIFIERS_H