#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/NodeInfo.h"
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The panel that edits the node selected on the canvas. It shows a colour selector and one
    labelled number field for each of the node's settings: count limit, repeat value, switch
    count limit, sub-loop count limit, probability, and the velocity, pitch and channel of its
    MIDI note. Each field is bound to a property of the node's tree or of its note, with the
    limits NodeInfo.h gives, so editing a field writes the graph through the UndoManager. A
    button opens the TraversalRulesWindow. With nothing selected the panel shows a hint instead.

    It registers a listener with NodeManager::nodeSelectedListeners. When a node is selected it
    binds the fields to that node and shows only those the node has: a node without a MIDI note
    shows no note fields, and an alternative node has no sub-loop field. When a node is
    deselected it hides them all. The colour selector sets the node's colour through
    GraphState::setNodeColour.

    PluginEditor owns it, and MenuArea shows it when the node icon is selected.
*/

class Node;
class NodeCanvas;
class GraphState;

class NodeMenu : public juce::Component
{
public:

    NodeMenu(NodeCanvas& nodeCanvas, GraphState& graphState, juce::ValueTree colourPresets, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    int narrowestHeight() const;

    ArsNovaUI::IconButton editTraversalRulesButton;

private:

    void bindToNode(const Node* node);

    enum class RowSource { NodeTree, SubLoop, MidiNote };

    struct LabeledRow
    {
        ArsNovaUI::LabeledEditor& field;
        juce::String              labelText;

        juce::Identifier          property;
        RowSource                 source;
        int                       minimum;
        int                       maximum;
        juce::String              prefix;
        juce::String              suffix;
    };

    GraphState&        graphState;
    juce::UndoManager& undoManager;

    ArsNovaUI::ColourSelector colourSelector;
    ArsNovaUI::CaptionLabel   colourLabel;

    int selectedNodeId = -1;

    ArsNovaUI::LabeledEditor countLimitField        { undoManager };
    ArsNovaUI::LabeledEditor repeatField            { undoManager };
    ArsNovaUI::LabeledEditor switchCountLimitField  { undoManager };
    ArsNovaUI::LabeledEditor subLoopCountLimitField { undoManager };
    ArsNovaUI::LabeledEditor probabilityField       { undoManager };
    ArsNovaUI::LabeledEditor velocityField          { undoManager };
    ArsNovaUI::LabeledEditor pitchField             { undoManager };
    ArsNovaUI::LabeledEditor channelField           { undoManager };

    std::array<LabeledRow, 8> labeledRows {{
        { countLimitField,        "CNT", ValueTreeIdentifiers::CountLimit,        RowSource::NodeTree, NodeInfo::minimumCountLimit,   NodeInfo::maximumCountLimit,   "",  ""  },
        { repeatField,            "RPT", ValueTreeIdentifiers::RepeatValue,       RowSource::NodeTree, NodeInfo::minimumRepeatValue,  NodeInfo::maximumRepeatValue,  "x", ""  },
        { switchCountLimitField,  "SW",  ValueTreeIdentifiers::SwitchCountLimit,  RowSource::NodeTree, NodeInfo::minimumCountLimit,   NodeInfo::maximumCountLimit,   "",  ""  },
        { subLoopCountLimitField, "SUB", ValueTreeIdentifiers::SubLoopCountLimit, RowSource::SubLoop,  NodeInfo::minimumCountLimit,   NodeInfo::maximumCountLimit,   "",  ""  },
        { probabilityField,       "PRB", ValueTreeIdentifiers::Probability,       RowSource::NodeTree, NodeInfo::minimumProbability,  NodeInfo::maximumProbability,  "",  "%" },
        { velocityField,          "VEL", ValueTreeIdentifiers::MidiVelocity,      RowSource::MidiNote, NodeInfo::minimumMidiVelocity, NodeInfo::maximumMidiVelocity, "",  ""  },
        { pitchField,             "PIT", ValueTreeIdentifiers::MidiPitch,         RowSource::MidiNote, NodeInfo::minimumMidiPitch,    NodeInfo::maximumMidiPitch,    "",  ""  },
        { channelField,           "CH",  ValueTreeIdentifiers::MidiChannel,       RowSource::MidiNote, NodeInfo::minimumMidiChannel,  NodeInfo::maximumMidiChannel,  "",  ""  }
    }};
};
