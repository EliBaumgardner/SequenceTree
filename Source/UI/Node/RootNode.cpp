//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "NodeTextEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeIdentifiers.h"

RootNode::RootNode(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::Root;

    setPaintingIsUnclipped(true);

    rootRectangle = std::make_unique<RootRectangle>(context);
    addAndMakeVisible(rootRectangle.get());
}

RootNode::~RootNode() = default;

void RootNode::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawRootNode(g, *this);
}

void RootNode::resized() {
    const int rw = loopLimitRectangleWidth;

    juce::Rectangle<int> bounds = getLocalBounds();

    int rectHeight = bounds.getHeight() / 2;
    int rectY      = (bounds.getHeight() - rectHeight) / 2;
    rootRectangle->setBounds(0, rectY, rw + 8, rectHeight);

    juce::Rectangle<int> circleArea = bounds.withTrimmedLeft(rw);

    juce::Rectangle<int> editorArea = circleArea.reduced(10);
    upButton.setBounds(editorArea.removeFromTop(4));
    downButton.setBounds(editorArea.removeFromBottom(4));
    nodeTextEditor->setBounds(editorArea);
    nodeTextEditor->setJustification(juce::Justification::centred);

    countEditor.setBounds(circleArea.getRight() - 18, circleArea.getY(), 18, 12);
}

void RootNode::setDisplayMode(NodeDisplayMode mode) {
    Node::setDisplayMode(mode);

    if (nodeValueTree.isValid() && rootRectangle != nullptr)
        rootRectangle->loopLimitEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::LoopLimit);
}
