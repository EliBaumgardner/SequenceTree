#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include "../Editors/LabeledEditor.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/NodeInfo.h"
#include "ColourSelector.h"
#include "../Buttons/IconButton.h"
#include "TraversalRulesWindow.h"
#include "../PopupWindow.h"
#include "../Theme/CustomLookAndFeel.h"

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

private:

    enum class RowSource { NodeTree, SubLoop, MidiNote };

    struct LabeledRow
    {
        LabeledEditor&   field;
        juce::String     labelText;
        juce::String     tooltip;
        juce::Identifier property;
        RowSource        source;
        int              minimum;
        int              maximum;
        juce::String     prefix;
        juce::String     suffix;
    };

    void bindToNode(const Node* node);

    GraphState&             graphState;
    TraversalRuleState&     traversalRuleState;
    AudioSnapshotPublisher& snapshots;
    juce::UndoManager&      undoManager;

    ColourSelector colourSelector;
    CaptionLabel   colourLabel;

    int selectedNodeId = -1;

    LabeledEditor countLimitField        { undoManager };
    LabeledEditor repeatField            { undoManager };
    LabeledEditor switchCountLimitField  { undoManager };
    LabeledEditor subLoopCountLimitField { undoManager };
    LabeledEditor probabilityField       { undoManager };
    LabeledEditor velocityField          { undoManager };
    LabeledEditor pitchField             { undoManager };
    LabeledEditor channelField           { undoManager };

    std::array<LabeledRow, 8> labeledRows {{
        { countLimitField,        "CNT", "Count Limit",          ValueTreeIdentifiers::CountLimit,        RowSource::NodeTree, minimumCountLimit,   maximumCountLimit,   "",  ""  },
        { repeatField,            "RPT", "Repeat Value",         ValueTreeIdentifiers::RepeatValue,       RowSource::NodeTree, minimumRepeatValue,  maximumRepeatValue,  "x", ""  },
        { switchCountLimitField,  "SW",  "Switch Count Limit",   ValueTreeIdentifiers::SwitchCountLimit,  RowSource::NodeTree, minimumCountLimit,   maximumCountLimit,   "",  ""  },
        { subLoopCountLimitField, "SUB", "Sub Loop Count Limit", ValueTreeIdentifiers::SubLoopCountLimit, RowSource::SubLoop,  minimumCountLimit,   maximumCountLimit,   "",  ""  },
        { probabilityField,       "PRB", "Probability",          ValueTreeIdentifiers::Probability,       RowSource::NodeTree, minimumProbability,  maximumProbability,  "",  "%" },
        { velocityField,          "VEL", "Velocity",             ValueTreeIdentifiers::MidiVelocity,      RowSource::MidiNote, minimumMidiVelocity, maximumMidiVelocity, "",  ""  },
        { pitchField,             "PIT", "Pitch",                ValueTreeIdentifiers::MidiPitch,         RowSource::MidiNote, minimumMidiPitch,    maximumMidiPitch,    "",  ""  },
        { channelField,           "CH",  "Channel",              ValueTreeIdentifiers::MidiChannel,       RowSource::MidiNote, minimumMidiChannel,  maximumMidiChannel,  "",  ""  }
    }};

    IconButton editTraversalRulesButton;

    PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesWindow>(CustomLookAndFeel::get(*this), traversalRuleState, snapshots, undoManager);

            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };
};
