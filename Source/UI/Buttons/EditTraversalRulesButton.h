//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_EDITTRAVERSALRULESBUTTON_H
#define SEQUENCETREE_EDITTRAVERSALRULESBUTTON_H

#include <memory>

#include "MenuTextButton.h"

class TraversalRulesWindow;

class EditTraversalRulesButton : public MenuTextButton {

public:

    explicit EditTraversalRulesButton(ApplicationContext& context);
    ~EditTraversalRulesButton() override;

private:

    void showTraversalRulesWindow();

    ApplicationContext& applicationContext;

    std::unique_ptr<TraversalRulesWindow> traversalRulesWindow;
};

#endif //SEQUENCETREE_EDITTRAVERSALRULESBUTTON_H
