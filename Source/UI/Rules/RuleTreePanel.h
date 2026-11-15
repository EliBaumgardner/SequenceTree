#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Buttons/IconButton.h>

#include "RuleTreeView.h"

class RuleTreePanel : public juce::Component
{
public:

    RuleTreePanel(TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;
    void resized () override;

    void lookAndFeelChanged() override;

    static constexpr int headerInset     = 3;
    static constexpr int separatorHeight = 1;

    RuleTreeView treeView;

private:

    void showMoreMenu();

    ArsNovaUI::IconButton addButton;
    ArsNovaUI::IconButton moreButton;

    juce::TextEditor filterEditor;

    juce::Viewport viewport;
};
