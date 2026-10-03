#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Util/ApplicationContext.h"
#include "../Bars/Bar.h"
#include "../Buttons/ButtonPane.h"
#include "../Buttons/IconButton.h"
#include "../PanelResizer.h"
#include "../LabelPanel.h"
#include "../Editors/FilePage.h"

class TraversalRulesWindow : public juce::Component,
                             private juce::Timer,
                             private juce::ValueTree::Listener,
                             private juce::AsyncUpdater
{
public:

    class RulesTitlebar : public Bar
    {
    public:

        explicit RulesTitlebar(const ApplicationContext& context);

        static constexpr int   preferredHeight    = 28;
        static constexpr float titlebarInsetRatio = 0.143f;

        IconButton playButton;

    private:

        void resized() override;

        ButtonPane undoRedoPane;
    };

    explicit TraversalRulesWindow(const ApplicationContext& context);
    ~TraversalRulesWindow() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void setActivePage(int id);

    static constexpr int defaultWidth  = 360;
    static constexpr int defaultHeight = 260 + RulesTitlebar::preferredHeight;

    static constexpr int minContentWidth = 80;
    static constexpr int statusBarHeight = 18;

    static constexpr int compileDelayMs = 250;

private:

    class RulesPanel : public juce::Component
    {
    public:

        explicit RulesPanel(const ApplicationContext& context);
        ~RulesPanel() override;

        void paint(juce::Graphics& graphics) override;
        void resized() override;

        void addLabel(int fileId, const juce::String& name);
        void removeLabel(int fileId);
        void selectLabel(int fileId);

        static constexpr int resizerWidth      = 10;
        static constexpr int minPanelWidth     = 60;
        static constexpr int defaultPanelWidth = 120;

        class PanelTitlebar : public Bar
        {
        public:

            explicit PanelTitlebar(const ApplicationContext& context);

            IconButton addButton;

        private:

            void resized() override;
        };

        PanelResizer  resizer;
        LabelPanel    labelPanel;
        PanelTitlebar panelTitlebar;
    };

    void setPanelWidth(int newWidth);
    int  clampPanelWidth(int newWidth) const;
    void compileViewedPage();
    void setStatus(const juce::String& text, bool isError);
    void addRule();
    void syncWithRuleState();
    void createPage(int ruleId, const juce::String& source);
    void makeViewedRuleActive();
    void removeRule(int ruleId);
    void timerCallback() override;
    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;
    void handleAsyncUpdate() override;

    RulesTitlebar titlebar;
    RulesPanel    rulesPanel;

    const ApplicationContext& context;

    std::unordered_map<int, std::unique_ptr<FilePage>> filePages;

    FilePage* activePage   = nullptr;
    int       viewedRuleId = -1;

    juce::String statusText;
    bool         statusIsError = false;

    int panelWidth = RulesPanel::defaultPanelWidth;
};
