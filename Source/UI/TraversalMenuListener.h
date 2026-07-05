//
// Created by Eli Baumgardner on 7/4/26.
//

#ifndef SEQUENCETREE_TRAVERSALMENULISTENER_H
#define SEQUENCETREE_TRAVERSALMENULISTENER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "TraversalMenu.h"
#include "../Graph/ValueTreeIdentifiers.h"

class TraversalMenuListener : public juce::ValueTree::Listener {

public:

    explicit TraversalMenuListener(TraversalMenu& traversalMenu) : traversalMenu(traversalMenu) {}

    void valueTreeChildAdded(juce::ValueTree& parent, juce::ValueTree& child) override {

        if (child.getType() == ValueTreeIdentifiers::TraversalData) {

            int traversalId = child.getProperty(ValueTreeIdentifiers::TraversalId);
            traversalMenu.displayMenu.addTraversalToMenu(traversalId);

            if (traversalMenu.displayMenu.selectedOption.isEmpty()) {
                traversalMenu.selectTraversal(traversalId);
            }
        }
    }

    void valueTreeChildRemoved(juce::ValueTree& parent, juce::ValueTree& child, int childIndex) override {

        if (child.getType() == ValueTreeIdentifiers::TraversalData) {

            int traversalId = child.getProperty(ValueTreeIdentifiers::TraversalId);
            traversalMenu.displayMenu.removeTraversalFromMenu(traversalId);
        }
    }

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& propertyIdentifier) override {}

private:

    TraversalMenu& traversalMenu;
};

#endif // SEQUENCETREE_TRAVERSALMENULISTENER_H
