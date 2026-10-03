#pragma once

#include "Node.h"
#include <juce_data_structures/juce_data_structures.h>
#include <vector>

class Encapsulator : public Node
{
public:

    explicit Encapsulator(const ApplicationContext& context);

    void bindToTree() override;
    void bindValueEditorForMode() override;
    void syncHighlightsFromMembers();

    std::vector<int> memberNodeIds;
    juce::ValueTree  firstMemberValueTree;
    bool             isExpanded = false;
};
