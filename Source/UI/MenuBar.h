//
// Created by Eli Baumgardner on 7/20/26.
//

#ifndef SEQUENCETREE_MENUBAR_H
#define SEQUENCETREE_MENUBAR_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Util/ApplicationContext.h"
#include "../UI/Theme/CustomLookAndFeel.h"
#include "Buttons/IconButton.h"

class MenuBar : public juce::Component {
public:

    MenuBar(ApplicationContext& context);
    void paint(juce::Graphics& g) override;
    void resized() override;

    ApplicationContext& context;

    std::unique_ptr<IconButton> treeIcon = nullptr;
    std::unique_ptr<IconButton> nodeIcon = nullptr;
    std::unique_ptr<IconButton> traversalIcon = nullptr;
};


#endif //SEQUENCETREE_MENUBAR_H