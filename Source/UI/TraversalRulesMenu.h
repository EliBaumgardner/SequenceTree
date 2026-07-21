//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_TRAVERSALRULESMENU_H
#define SEQUENCETREE_TRAVERSALRULESMENU_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Util/ApplicationContext.h"

class TraversalRulesMenu : public juce::Component {

public:

    explicit TraversalRulesMenu(ApplicationContext& context);
    ~TraversalRulesMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int defaultWidth  = 360;
    static constexpr int defaultHeight = 260;

private:

    ApplicationContext& applicationContext;
};

class TraversalRulesWindow : public juce::DocumentWindow {

public:

    explicit TraversalRulesWindow(ApplicationContext& context);

    void closeButtonPressed() override;
};

#endif //SEQUENCETREE_TRAVERSALRULESMENU_H
