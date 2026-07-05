//
// Created by Eli Baumgardner on 5/23/26.
//

#ifndef SEQUENCETREE_TRAVERSALDISPLAYMENU_H
#define SEQUENCETREE_TRAVERSALDISPLAYMENU_H

#include "Buttons/DisplayMenu.h"

struct ApplicationContext;

class TraversalDisplayMenu : public DisplayMenu {

public:

    explicit TraversalDisplayMenu(ApplicationContext& context);

    void paint(juce::Graphics& g) override;

    void addTraversalToMenu(int traversalId);
    void removeTraversalFromMenu(int traversalId);

    std::function<void(int)> onTraversalSelected;

private:

    ApplicationContext& applicationContext;
};

#endif //SEQUENCETREE_TRAVERSALDISPLAYMENU_H
