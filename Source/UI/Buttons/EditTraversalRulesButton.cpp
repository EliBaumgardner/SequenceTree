//
// Created by Eli Baumgardner on 7/21/26.
//

#include "EditTraversalRulesButton.h"
#include "../TraversalRulesMenu.h"

EditTraversalRulesButton::EditTraversalRulesButton(ApplicationContext& context)
    : MenuTextButton(context, "edit traversal rules"), applicationContext(context)
{
    onClick = [this] { showTraversalRulesWindow(); };
}

EditTraversalRulesButton::~EditTraversalRulesButton() = default;

void EditTraversalRulesButton::showTraversalRulesWindow() {
    if (traversalRulesWindow == nullptr) {
        traversalRulesWindow = std::make_unique<TraversalRulesWindow>(applicationContext);
    }

    traversalRulesWindow->centreWithSize(traversalRulesWindow->getWidth(),
                                         traversalRulesWindow->getHeight());
    traversalRulesWindow->setVisible(true);
    traversalRulesWindow->toFront(true);
}
