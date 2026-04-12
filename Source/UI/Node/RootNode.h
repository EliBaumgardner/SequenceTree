//
// Created by Eli Baumgardner on 4/11/26.
//

#ifndef SEQUENCETREE_ROOTNODE_H
#define SEQUENCETREE_ROOTNODE_H


#include "Node.h"
#include "Buttons/RootRectangle.h"

//TODO 1. implement CustomlookandFeel
//TODO 2. refactor audio engine for root node
//TODO 3. Make Root Node creation automatic
//TODO 4. Implement nodeController functions for attaching node arrrows to the roots
//TODO 5. create button to control loop functionality
//TODO 6. implement functionality in audio engine


class RootNode : public Node {

    public:
    RootNode();
    ~RootNode() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::unique_ptr<RootRectangle> rootRectangle = nullptr;
};


#endif //SEQUENCETREE_ROOTNODE_H