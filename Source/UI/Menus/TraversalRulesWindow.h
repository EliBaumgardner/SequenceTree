#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/PanelResizer.h>
#include <ArsNovaUI/LabelPanel.h>
#include <ArsNovaUI/Editors/FilePage.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

/*
    The window for writing traversal rule scripts. A side panel lists the rules by name and has an
    add button, and the rest of the window is a code editor (a FilePage) for the rule that is
    selected, with a status bar along the bottom. A rule is a script in TraversalRuleState's rules
    tree, and one of them is the live rule that the audio thread runs.

    Typing in a page stores its text in TraversalRuleState at once and starts a 250 ms timer.
    When the timer fires, compileViewedPage compiles the text with compileTraversalScript, marks
    the error lines, and reports either the first error or the instruction count. If the viewed
    rule is the live one, a successful compile is published through
    AudioSnapshotPublisher::publishScript; a failed one leaves the last good script playing. The
    play button in the title bar makes the viewed rule the live one. Removing a rule drops its
    page, and republishes the live rule if the removed one was live. The window listens to the
    rules tree and refreshes its pages and list when it changes from elsewhere, for example by a
    restored patch.

    TraversalMenu and NodeMenu open it in a pop-up window, so it takes the CustomLookAndFeel in
    its constructor and sets it on itself.
*/

class TraversalRuleState;
class AudioSnapshotPublisher;

class TraversalRulesWindow : public juce::Component,
                             private juce::Timer,
                             private juce::ValueTree::Listener,
                             private juce::AsyncUpdater
{
public:

    TraversalRulesWindow(ArsNovaUI::CustomLookAndFeel& lookAndFeel, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                         juce::UndoManager& undoManager);

    ~TraversalRulesWindow() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void setActivePage(int id);

    class RulesTitlebar : public ArsNovaUI::Bar
    {
    public:

        RulesTitlebar();

        static constexpr int   preferredHeight    = 28;
        static constexpr float titlebarInsetRatio = 0.143f;

        ArsNovaUI::IconButton playButton;
        ArsNovaUI::ButtonPane undoRedoPane;

    private:

        void resized() override;
    };

    static constexpr int defaultWidth  = 360;
    static constexpr int defaultHeight = 260 + RulesTitlebar::preferredHeight;

    static constexpr int minContentWidth = 80;
    static constexpr int statusBarHeight = 18;

    static constexpr int compileDelayMs = 250;

private:

    void addRule             ();
    void removeRule          (int ruleId);
    void makeViewedRuleActive();
    void syncWithRuleState   ();

    void createPage       (int ruleId, const juce::String& source);
    void compileViewedPage();
    void setStatus        (const juce::String& text, bool isError);

    void setPanelWidth(int newWidth);

    void timerCallback    () override;
    void handleAsyncUpdate() override;

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    int clampPanelWidth(int newWidth) const;

    class RulesPanel : public juce::Component
    {
    public:

        explicit RulesPanel(juce::UndoManager& undoManager);

        void paint(juce::Graphics& graphics) override;
        void resized() override;

        void addLabel   (int fileId, const juce::String& name);
        void removeLabel(int fileId);
        void selectLabel(int fileId);

        class PanelTitlebar : public ArsNovaUI::Bar
        {
        public:

            PanelTitlebar();

            ArsNovaUI::IconButton addButton;

        private:

            void resized() override;
        };

        static constexpr int resizerWidth      = 10;
        static constexpr int minPanelWidth     = 60;
        static constexpr int defaultPanelWidth = 120;

        ArsNovaUI::PanelResizer resizer;
        ArsNovaUI::LabelPanel   labelPanel;
        PanelTitlebar           panelTitlebar;
    };

    RulesTitlebar titlebar;
    RulesPanel    rulesPanel;

    TraversalRuleState&     traversalRuleState;
    AudioSnapshotPublisher& snapshots;

    std::unordered_map<int, std::unique_ptr<ArsNovaUI::FilePage>> filePages;

    ArsNovaUI::FilePage* activePage   = nullptr;
    int                  viewedRuleId = -1;

    juce::String statusText;
    bool         statusIsError = false;

    int panelWidth = RulesPanel::defaultPanelWidth;
};
