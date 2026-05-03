//
// Created by Eli Baumgardner on 4/11/26.
//

#ifndef SEQUENCETREE_ROOTNODE_H
#define SEQUENCETREE_ROOTNODE_H


#include "Node.h"
#include "../Buttons/RootRectangle.h"

//TODO 1. implement CustomlookandFeel
//TODO 2. refactor audio engine for root node
//TODO 3. Make Root Node creation automatic
//TODO 4. Implement nodeController functions for attaching node arrrows to the roots
//TODO 5. create button to control loop functionality
//TODO 6. implement functionality in audio engine


class RootNode : public Node {

    public:
    // Width of the loop-limit rectangle that sits to the left of the circle.
    // The component extends left by this amount so the rectangle fits without clipping.
    static constexpr int loopLimitRectangleWidth = 10;

    RootNode(ApplicationContext& context);
    ~RootNode() override;
    void paint(juce::Graphics& g) override;
    void resized() override;
    void setDisplayMode(NodeDisplayMode mode) override;

    juce::Point<int> getNodeCentre() const override
    {
        // Circle centre is loopLimitRectangleWidth pixels to the right of the component left edge,
        // then R = getHeight()/2 more to the right to reach the circle centre.
        return { getBounds().getX() + loopLimitRectangleWidth + getHeight() / 2,
                 getBounds().getCentreY() };
    }

    std::unique_ptr<RootRectangle> rootRectangle = nullptr;
};


#endif //SEQUENCETREE_ROOTNODE_H