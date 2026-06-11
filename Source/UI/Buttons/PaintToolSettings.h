//
// Created by Eli Baumgardner on 6/10/26.
//

#ifndef SEQUENCETREE_PAINTTOOLSETTINGS_H
#define SEQUENCETREE_PAINTTOOLSETTINGS_H

#include <juce_graphics/juce_graphics.h>
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "DisplayMenu.h"


class PaintToolDisplayMenu : public DisplayMenu {
public:

    PaintToolDisplayMenu(ApplicationContext& context) : DisplayMenu(context) {};

    void paint(juce::Graphics& g) {
        CustomLookAndFeel::get(*this).drawDisplayMenu(g, *this);
    };
};

class PaintToolSettings : public juce::Component {

public:

    ApplicationContext& context;
    std::unique_ptr<PaintToolDisplayMenu> displayMenu;

    PaintToolSettings(ApplicationContext context) : context(context) {
        setLookAndFeel(context.lookAndFeel);

        displayMenu = std::make_unique<PaintToolDisplayMenu>(context);
        addAndMakeVisible(displayMenu.get());
    };

    void paint(juce::Graphics& g) {
        CustomLookAndFeel::get(*this).drawPaintToolSettings(g,*this);
    };

    void resized() {

        int height = getLocalBounds().getHeight();
        int displayMenuHeight = height * 0.2f;
        displayMenu->setBounds(getLocalBounds().removeFromTop(displayMenuHeight));
    };
};

#endif SEQUENCETREE_PAINTTOOLSETTINGS_H