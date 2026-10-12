#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/PanelResizer.h>
#include <ArsNovaUI/LabelPanel.h>
#include <ArsNovaUI/Editors/FilePage.h>

class CustomLookAndFeel;
class TraversalRuleState;
class AudioSnapshotPublisher;

class TraversalRulesWindow : public juce::Component,
                             private juce::Timer,
                             private juce::ValueTree::Listener,
                             private juce::AsyncUpdater
{
public:

    TraversalRulesWindow(CustomLookAndFeel& lookAndFeel, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                         juce::UndoManager& undoManager);

    ~TraversalRulesWindow() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void setActivePage(int id);

    class RulesTitlebar : public Bar
    {
    public:

        RulesTitlebar();

        static constexpr int   preferredHeight    = 28;
        static constexpr float titlebarInsetRatio = 0.143f;

        IconButton playButton;
        ButtonPane undoRedoPane;

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

        class PanelTitlebar : public Bar
        {
        public:

            PanelTitlebar();

            IconButton addButton;

        private:

            void resized() override;
        };

        static constexpr int resizerWidth      = 10;
        static constexpr int minPanelWidth     = 60;
        static constexpr int defaultPanelWidth = 120;

        PanelResizer  resizer;
        LabelPanel    labelPanel;
        PanelTitlebar panelTitlebar;
    };

    RulesTitlebar titlebar;
    RulesPanel    rulesPanel;

    TraversalRuleState&     traversalRuleState;
    AudioSnapshotPublisher& snapshots;

    std::unordered_map<int, std::unique_ptr<FilePage>> filePages;

    FilePage* activePage   = nullptr;
    int       viewedRuleId = -1;

    juce::String statusText;
    bool         statusIsError = false;

    int panelWidth = RulesPanel::defaultPanelWidth;
};
