//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_TRAVERSALRULESWINDOW_H
#define SEQUENCETREE_TRAVERSALRULESWINDOW_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "../../Util/ApplicationContext.h"
#include "../Bar.h"
#include "../Buttons/ButtonPane.h"
#include "../Buttons/IconButton.h"
#include "../PanelResizer.h"
#include "../LabelPanel.h"
#include "../Editors/FilePage.h"

class TraversalRulesWindow : public juce::Component,
                             private juce::Timer {

public:

    class RulesTitlebar : public Bar {

    public:

        explicit RulesTitlebar(ApplicationContext& context);

        std::function<void()> onPlayClicked;

        static constexpr int preferredHeight = 28;

    private:

        void paintOverBar(juce::Graphics& g) override;
        void resized() override;

        void configureUndoRedoPane();

        std::unique_ptr<IconButton> playButton;
        ButtonPane                  undoRedoPane;
    };

    explicit TraversalRulesWindow(ApplicationContext& context);
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

        explicit RulesPanel(ApplicationContext& context);
        ~RulesPanel() override;

        std::function<void(int)> propagateLabelClicked;
        std::function<void(int)> propagateLabelRemoved;
        std::function<void()>    propagateAddClicked;

        void addLabel   (int fileId, const juce::String& name);
        void selectLabel(int fileId);

        void paint(juce::Graphics& g) override;
        void resized() override;

        static constexpr int resizerWidth      = 10;
        static constexpr int minPanelWidth     = 60;
        static constexpr int defaultPanelWidth = 120;

        PanelResizer resizer;

    private:

        class PanelTitlebar : public Bar {

        public:

            explicit PanelTitlebar(ApplicationContext& context);

            std::function<void()> onAddClicked;
        private:

            void resized() override;

            std::unique_ptr<IconButton> addButton;
        };

        PanelTitlebar               panelTitlebar;
        std::unique_ptr<LabelPanel> labelPanel = nullptr;

    };

    void timerCallback() override;

    void loadRules();
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

    ApplicationContext& context;

    std::unordered_map<int, std::unique_ptr<FilePage>> filePages;
    FilePage* activePage   = nullptr;
    int       viewedRuleId = -1;

    juce::String statusText;
    bool         statusIsError = false;

    int panelWidth = RulesPanel::defaultPanelWidth;


};

#endif //SEQUENCETREE_TRAVERSALRULESWINDOW_H
