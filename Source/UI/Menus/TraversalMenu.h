#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Menus/ItemSelector.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include "TraversalRulesWindow.h"
#include <ArsNovaUI/PopupWindow.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

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

    int narrowestHeight() const;

    ArsNovaUI::ItemSelector displayMenu;

    ArsNovaUI::LabeledEditor multiplierField;
    ArsNovaUI::LabeledEditor channelField;
    ArsNovaUI::LabeledEditor transposeField;
    ArsNovaUI::LabeledEditor velocityField;

    ArsNovaUI::CaptionLabel   colourLabel;
    ArsNovaUI::ColourSelector colourSelector;

    ArsNovaUI::IconButton editTraversalRulesButton;

    ArsNovaUI::PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesWindow>(ArsNovaUI::CustomLookAndFeel::get(*this), traversalRuleState, snapshots, undoManager);

            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };

private:

    void valueTreeChildAdded  (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;

    NodeCanvas&             nodeCanvas;
    GraphState&             graphState;
    TraversalRuleState&     traversalRuleState;
    AudioSnapshotPublisher& snapshots;
    juce::UndoManager&      undoManager;

    juce::ValueTree currentTraversalData;
};
