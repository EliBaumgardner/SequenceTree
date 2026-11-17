#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/PanelResizer.h>
#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Editors/FilePage.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <ArsNovaUI/Bars/PathBar.h>
#include "RuleTabBar.h"
#include "RuleTreePanel.h"
#include "Core/Script/ScriptCompiler.h"

/*
    The window for writing traversal rule scripts, laid out like a code editor. The side panel on
    the left is a RuleTreePanel, the rules in their folders, where rules and folders are added,
    renamed, moved by dragging and deleted; a PanelResizer along its inner edge sets its width.
    Open rules are tabs in a RuleTabBar, which also holds the play, undo and redo buttons. A
    PathBar under it shows the open rule's folders, its name and the function the caret is
    in, and the rest is a code editor (a FilePage) for the open rule, with a status bar along the
    bottom.
    Adding, renaming, moving and deleting rules and folders go through the UndoManager it is
    given. Typing in a page does not: the page keeps its own undo history, which the undo and
    redo buttons act on.

    Typing in a page stores its text in TraversalRuleState at once and starts a 250 ms timer.
    When the timer fires, compileViewedPage compiles the text with compileTraversalScript, marks
    the error lines, keeps the outline, and reports either the first error or the instruction
    count. The window never publishes: AudioSnapshotPublisher listens to the same trees and
    publishes the live rule, 250 ms after typing in it pauses and at once when another rule
    becomes the live one. The play button, or Make live in the tree's menu, makes a rule the live
    one. Removing a rule drops its page and tab; removing a folder never removes the live rule.
    The window listens to the rules and folders trees and refreshes its pages, tree and tabs when
    they change from elsewhere, for example by a restored patch.

    PluginEditor opens it in a pop-up window from TraversalMenu's and NodeMenu's buttons, so it
    takes the CustomLookAndFeel in its constructor and sets it on itself.
*/

class TraversalRuleState;

class TraversalRulesWindow : public juce::Component,
                             private juce::Timer,
                             private juce::ValueTree::Listener,
                             private juce::AsyncUpdater
{
public:

    TraversalRulesWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, TraversalRuleState& traversalRuleState, juce::UndoManager& undoManager);

    ~TraversalRulesWindow() override;

    void paint            (juce::Graphics& graphics) override;
    void paintOverChildren(juce::Graphics& graphics) override;
    void resized () override;

    void openRule(int ruleId);

    static constexpr int defaultWidth      = 600;
    static constexpr int defaultHeight     = 360;
    static constexpr int minContentWidth   = 200;
    static constexpr int statusBarHeight   = 18;
    static constexpr int minPanelWidth     = 120;
    static constexpr int defaultPanelWidth = 190;
    static constexpr int compileDelayMs    = 250;

    static constexpr float resizerWidthRatio = 0.167f;

private:

    void addRule     (int folderId);
    void removeRule  (int ruleId);
    void removeFolder(int folderId);
    void makeRuleLive(int ruleId);
    void syncWithRuleState ();
    void refreshTabs ();

    void viewRule(int ruleId);
    void closeTab(int ruleId);

    void createPage   (int ruleId, const juce::String& source);
    void compileViewedPage ();
    void showCaretPosition ();
    void setStatus    (const juce::String& text, bool isError);
    void setPanelWidth(int newWidth);

    void timerCallback    () override;
    void handleAsyncUpdate() override;

    void valueTreeChildAdded       (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved     (juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreeChildOrderChanged(juce::ValueTree& parent, int oldIndex, int newIndex) override;
    void valueTreePropertyChanged  (juce::ValueTree& tree, const juce::Identifier& property) override;

    int clampPanelWidth(int newWidth) const;
    int outlineEntryAt (int line) const;

    ArsNovaUI::PanelResizer resizer;
    juce::Component         sidePanel;

    ArsNovaUI::Bar          panelBar;
    ArsNovaUI::CaptionLabel panelTitle;

    RuleTreePanel treePanel;

    RuleTabBar  tabBar;
    ArsNovaUI::PathBar pathBar;

    TraversalRuleState& traversalRuleState;
    juce::UndoManager&  undoManager;

    std::unordered_map<int, std::unique_ptr<ArsNovaUI::FilePage>> filePages;

    ArsNovaUI::FilePage* activePage = nullptr;

    ScriptOutline viewedOutline;

    juce::Rectangle<int> editorArea;

    juce::String statusText;
    juce::String caretText;

    int viewedRuleId = -1;
    int panelWidth   = defaultPanelWidth;

    bool statusIsError = false;
};
