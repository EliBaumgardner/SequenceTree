#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/NodeInfo.h"
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include "TraversalRulesWindow.h"
#include <ArsNovaUI/PopupWindow.h>
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

    MenuArea owns it and shows it when the node icon is selected.
*/

class Node;
class NodeCanvas;
class GraphState;
class TraversalRuleState;
class AudioSnapshotPublisher;

class NodeMenu : public juce::Component
{
public:

    NodeMenu(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
             juce::ValueTree colourPresets, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    int narrowestHeight() const;

private:

    void bindToNode(const Node* node);

    enum class RowSource { NodeTree, SubLoop, MidiNote };

    struct LabeledRow
    {
        ArsNovaUI::LabeledEditor& field;
        juce::String              labelText;
        juce::String              tooltip;
        juce::Identifier          property;
        RowSource                 source;
        int                       minimum;
        int                       maximum;
        juce::String              prefix;
        juce::String              suffix;
    };

    GraphState&             graphState;
    TraversalRuleState&     traversalRuleState;
    AudioSnapshotPublisher& snapshots;
    juce::UndoManager&      undoManager;

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
        { countLimitField,        "CNT", "Count Limit",          ValueTreeIdentifiers::CountLimit,        RowSource::NodeTree, NodeInfo::minimumCountLimit,   NodeInfo::maximumCountLimit,   "",  ""  },
        { repeatField,            "RPT", "Repeat Value",         ValueTreeIdentifiers::RepeatValue,       RowSource::NodeTree, NodeInfo::minimumRepeatValue,  NodeInfo::maximumRepeatValue,  "x", ""  },
        { switchCountLimitField,  "SW",  "Switch Count Limit",   ValueTreeIdentifiers::SwitchCountLimit,  RowSource::NodeTree, NodeInfo::minimumCountLimit,   NodeInfo::maximumCountLimit,   "",  ""  },
        { subLoopCountLimitField, "SUB", "Sub Loop Count Limit", ValueTreeIdentifiers::SubLoopCountLimit, RowSource::SubLoop,  NodeInfo::minimumCountLimit,   NodeInfo::maximumCountLimit,   "",  ""  },
        { probabilityField,       "PRB", "Probability",          ValueTreeIdentifiers::Probability,       RowSource::NodeTree, NodeInfo::minimumProbability,  NodeInfo::maximumProbability,  "",  "%" },
        { velocityField,          "VEL", "Velocity",             ValueTreeIdentifiers::MidiVelocity,      RowSource::MidiNote, NodeInfo::minimumMidiVelocity, NodeInfo::maximumMidiVelocity, "",  ""  },
        { pitchField,             "PIT", "Pitch",                ValueTreeIdentifiers::MidiPitch,         RowSource::MidiNote, NodeInfo::minimumMidiPitch,    NodeInfo::maximumMidiPitch,    "",  ""  },
        { channelField,           "CH",  "Channel",              ValueTreeIdentifiers::MidiChannel,       RowSource::MidiNote, NodeInfo::minimumMidiChannel,  NodeInfo::maximumMidiChannel,  "",  ""  }
    }};

    ArsNovaUI::IconButton editTraversalRulesButton;

    ArsNovaUI::PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesWindow>(ArsNovaUI::CustomLookAndFeel::get(*this), traversalRuleState, snapshots, undoManager);

            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };
};
