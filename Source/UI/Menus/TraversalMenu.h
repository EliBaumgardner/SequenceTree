#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "ItemSelector.h"
#include "../Editors/LabeledEditor.h"
#include "ColourSelector.h"
#include "../Buttons/IconButton.h"
#include "TraversalRulesWindow.h"
#include "../PopupWindow.h"
#include "../Theme/CustomLookAndFeel.h"

class NodeCanvas;
class GraphState;
class TraversalRuleState;
class AudioSnapshotPublisher;

class TraversalMenu : public juce::Component, private juce::ValueTree::Listener
{
public:
    TraversalMenu(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                  juce::ValueTree colourPresets, juce::UndoManager& undoManager);
    ~TraversalMenu() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void selectTraversal(int traversalId);

    ItemSelector displayMenu;

    LabeledEditor multiplierField;
    LabeledEditor channelField;
    LabeledEditor transposeField;
    LabeledEditor velocityField;

    CaptionLabel   colourLabel;
    ColourSelector colourSelector;

    IconButton editTraversalRulesButton;

    PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesWindow>(CustomLookAndFeel::get(*this), traversalRuleState, snapshots, undoManager);

            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };

private:

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    NodeCanvas&             nodeCanvas;
    GraphState&             graphState;
    TraversalRuleState&     traversalRuleState;
    AudioSnapshotPublisher& snapshots;
    juce::UndoManager&      undoManager;

    juce::ValueTree currentTraversalData;
};
