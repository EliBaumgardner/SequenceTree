//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_TRAVERSALRULESWINDOW_H
#define SEQUENCETREE_TRAVERSALRULESWINDOW_H

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
                             private juce::AsyncUpdater {

public:

    class RulesTitlebar : public Bar {

    public:

        explicit RulesTitlebar(const ApplicationContext& context);

        std::function<void()> onPlayClicked;

        static constexpr int   preferredHeight   = 28;
        static constexpr float contentInsetRatio = 0.143f;

    private:

        void paintOverBar(juce::Graphics& g) override;
        void resized() override;

        void configureUndoRedoPane();

        std::unique_ptr<IconButton> playButton;
        ButtonPane                  undoRedoPane;
    };

    explicit TraversalRulesWindow(const ApplicationContext& context);
    ~TraversalRulesWindow() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setActivePage(int id);

    static constexpr int defaultWidth  = 360;
    static constexpr int defaultHeight = 260 + RulesTitlebar::preferredHeight;

    static constexpr int minContentWidth = 80;
    static constexpr int statusBarHeight = 18;

    static constexpr int compileDelayMs = 250;

private:

    class RulesPanel : public juce::Component {

    public:

        explicit RulesPanel(const ApplicationContext& context);
        ~RulesPanel() override;

        std::function<void(int)>              propagateLabelClicked;
        std::function<void(int)>              propagateLabelRemoved;
        std::function<void()>                 propagateAddClicked;
        std::function<void(std::vector<int>)> propagateLabelsReordered;

        void addLabel   (int fileId, const juce::String& name);
        void removeLabel(int fileId);
        void selectLabel(int fileId);

        void paint(juce::Graphics& g) override;
        void resized() override;

        static constexpr int resizerWidth      = 10;
        static constexpr int minPanelWidth     = 60;
        static constexpr int defaultPanelWidth = 120;

        PanelResizer                resizer;
        std::unique_ptr<LabelPanel> labelPanel = nullptr;

    private:

        class PanelTitlebar : public Bar {

        public:

            explicit PanelTitlebar(const ApplicationContext& context);

            std::function<void()> onAddClicked;
        private:

            void resized() override;

            std::unique_ptr<IconButton> addButton;
        };

        PanelTitlebar panelTitlebar;

    };

    void timerCallback() override;

    void valueTreeChildAdded     (juce::ValueTree& parent, juce::ValueTree& child) override;
    void valueTreeChildRemoved   (juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override;
    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    void handleAsyncUpdate() override;

    void syncWithRuleState();
    void addRule();
    void removeRule(int ruleId);
    void createPage(int ruleId, const juce::String& source);

    void makeViewedRuleActive();
    void compileViewedPage();

    void setStatus(const juce::String& text, bool isError);

    juce::Rectangle<int> statusBarBounds() const;

    int  clampPanelWidth(int newWidth) const;
    void setPanelWidth(int newWidth);

    RulesTitlebar  titlebar;
    RulesPanel     rulesPanel;
    juce::Viewport filePageViewport;

    const ApplicationContext& context;

    std::unordered_map<int, std::unique_ptr<FilePage>> filePages;
    FilePage* activePage   = nullptr;
    int       viewedRuleId = -1;

    juce::String statusText;
    bool         statusIsError = false;

    int panelWidth = RulesPanel::defaultPanelWidth;


};

#endif //SEQUENCETREE_TRAVERSALRULESWINDOW_H
