//
// Created by Eli Baumgardner on 9/6/26.
//

#ifndef SEQUENCETREE_ENCAPSULATOR_H
#define SEQUENCETREE_ENCAPSULATOR_H

#include "Node.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class Encapsulator : public Node {

public:

    explicit Encapsulator(ApplicationContext& context);

    void paint(juce::Graphics& g) override;

    void bindToTree() override;
    void bindValueEditorForMode() override;

    void syncHighlightsFromMembers();

    std::vector<int> memberNodeIds;
    juce::ValueTree  firstMemberValueTree;
    bool             isExpanded = false;
};

#endif //SEQUENCETREE_ENCAPSULATOR_H
