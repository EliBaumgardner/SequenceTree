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
#include "../ResizablePanel.h"

class TraversalRulesWindow : public juce::Component {

public:

    class RulesTitlebar : public Bar {

    public:

        explicit RulesTitlebar(ApplicationContext& context);

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

    static constexpr int defaultWidth  = 360;
    static constexpr int defaultHeight = 260 + RulesTitlebar::preferredHeight;

    static constexpr int minContentWidth = 80;

private:

    class RulesPanel : public ResizablePanel {

    public:

        explicit RulesPanel(ApplicationContext& context);

        void paint(juce::Graphics& g) override;
        void resized() override;

        static constexpr int resizerWidth      = 10;
        static constexpr int minPanelWidth     = 60;
        static constexpr int defaultPanelWidth = 120;

    private:

        int minimumWidth() const override { return minPanelWidth; }
    };

    int  clampPanelWidth(int newWidth) const;
    void setPanelWidth(int newWidth);

    RulesTitlebar titlebar;
    RulesPanel    rulesPanel;

    int panelWidth = RulesPanel::defaultPanelWidth;
};

#endif //SEQUENCETREE_TRAVERSALRULESWINDOW_H
