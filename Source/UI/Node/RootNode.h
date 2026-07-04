//
// Created by Eli Baumgardner on 4/11/26.
//

#ifndef SEQUENCETREE_ROOTNODE_H
#define SEQUENCETREE_ROOTNODE_H


#include "Node.h"
#include "../Buttons/RootRectangle.h"


class RootNode : public Node {

    public:

    static constexpr int loopLimitRectangleWidth = 10;

    RootNode(ApplicationContext& context);
    ~RootNode() override;
    void paint(juce::Graphics& g) override;
    void resized() override;

    juce::Point<int> getNodeCentre() const override
    {
        return { getBounds().getX() + loopLimitRectangleWidth + getHeight() / 2,
                 getBounds().getCentreY() };
    }

    std::unique_ptr<RootRectangle> rootRectangle = nullptr;

};


#endif //SEQUENCETREE_ROOTNODE_H