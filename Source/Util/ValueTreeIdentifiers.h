//
// Created by Eli Baumgardner on 4/1/26.
//

#ifndef SEQUENCETREE_VALUETREEIDENTIFIERS_H
#define SEQUENCETREE_VALUETREEIDENTIFIERS_H

#include <juce_gui_basics/juce_gui_basics.h>


class ValueTreeIdentifiers {

public:

    static const juce::Identifier CanvasData;
    static const juce::Identifier NodeMap;
    static const juce::Identifier NodeTreeMap;

    static const juce::Identifier NodeTreeIds;
    static const juce::Identifier NodeTreeData;

    static const juce::Identifier RootNodeData;
    static const juce::Identifier NodeData;
    static const juce::Identifier ConnectorData;
    static const juce::Identifier ModulatorData;
    static const juce::Identifier ModulatorRootData;

    static const juce::Identifier NodeChildrenIds;

    static const juce::Identifier MidiNotesData;
    static const juce::Identifier MidiNoteData;

    // Property Identifiers

    static const juce::Identifier Id;

    static const juce::Identifier RootNodeId;
    static const juce::Identifier NodeId;
    static const juce::Identifier NodeTreeId;

    //Node Property Identifiers

    static const juce::Identifier CountLimit;
    static const juce::Identifier Count;

    static const juce::Identifier XPosition;
    static const juce::Identifier YPosition;
    static const juce::Identifier Radius;
    static const juce::Identifier ColourId;

    static const juce::Identifier MidiPitch;
    static const juce::Identifier MidiVelocity;
    static const juce::Identifier MidiDuration;

    static const juce::Identifier ModAmount;

    static const juce::Identifier ModulationType;
    static const juce::Identifier PitchMod;
    static const juce::Identifier VelocityMod;
    static const juce::Identifier DurationMod;


    //Static ValueTrees
};


#endif //SEQUENCETREE_VALUETREEIDENTIFIERS_H