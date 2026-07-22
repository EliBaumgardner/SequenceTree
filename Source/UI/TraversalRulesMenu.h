//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_TRAVERSALRULESMENU_H
#define SEQUENCETREE_TRAVERSALRULESMENU_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Util/ApplicationContext.h"
#include "ResizablePanel.h"

class TraversalRulesMenu : public ResizablePanel {

public:

    explicit TraversalRulesMenu(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int defaultWidth  = 360;
    static constexpr int defaultHeight = 260;

    static constexpr int resizerWidth      = 10;
    static constexpr int minPanelWidth     = 60;
    static constexpr int minContentWidth   = 80;
    static constexpr int defaultPanelWidth = 120;

private:

    int  clampPanelWidth(int newWidth) const;
    void setPanelWidth(int newWidth);

    int  resizeStartWidth() const override { return panelWidth; }
    void applyResizedWidth(int newWidth) override { setPanelWidth(newWidth); }

    class Panel : public juce::Component {
    public:
        void paint(juce::Graphics& g) override;
    };

    Panel panel;

    int panelWidth = defaultPanelWidth;
};

#endif //SEQUENCETREE_TRAVERSALRULESMENU_H
