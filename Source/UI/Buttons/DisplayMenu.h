//
// Created by Eli Baumgardner on 11/9/25.
//

#ifndef SEQUENCETREE_DISPLAYMENU_H
#define SEQUENCETREE_DISPLAYMENU_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomTextEditor.h"
#include "DisplayButton.h"

struct ApplicationContext;

class DisplayMenu : public juce::Component {

    ApplicationContext& applicationContext;

public:

    DisplayButton button;
    CustomTextEditor display;
    juce::PopupMenu menu;
    juce::String selectedOption = "";

    DisplayMenu(ApplicationContext& context);

    void paint(juce::Graphics& g) override;
    void resized() override;
};

#endif //SEQUENCETREE_DISPLAYMENU_H
