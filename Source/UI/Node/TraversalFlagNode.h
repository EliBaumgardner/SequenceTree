//
// Created by Eli Baumgardner on 6/30/26.
//

#ifndef SEQUENCETREE_TRAVERSALFLAGNODE_H
#define SEQUENCETREE_TRAVERSALFLAGNODE_H

#include "Node.h"

class TraversalFlagNode : public Node {

    public:

    TraversalFlagNode(ApplicationContext& context);
    void paint(juce::Graphics& g) override;
    void resized() override;
    bool hitTest(int x, int y) override;

    private:
    juce::Path buildTrianglePath() const;
    juce::Colour outlineColour = juce::Colours::black;

    std::unique_ptr<ValueEditor> traversalNumEditor = nullptr;
};

#endif //SEQUENCETREE_TRAVERSALFLAGNODE_H
