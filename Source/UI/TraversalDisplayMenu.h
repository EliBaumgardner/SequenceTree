//
// Created by Eli Baumgardner on 5/23/26.
//

#ifndef SEQUENCETREE_TRAVERSALDISPLAYMENU_H
#define SEQUENCETREE_TRAVERSALDISPLAYMENU_H

#include "Buttons/DisplayMenu.h"

struct ApplicationContext;

class TraversalDisplayMenu : public DisplayMenu {

public:

    TraversalDisplayMenu(ApplicationContext& context);

    void paint(juce::Graphics& g) override;

    void addTraversalToMenu(int traversalId);

private:

    ApplicationContext& applicationContext;
};

#endif //SEQUENCETREE_TRAVERSALDISPLAYMENU_H
