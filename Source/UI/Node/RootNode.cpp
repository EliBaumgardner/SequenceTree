//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "NodeTextEditor.h"
#include "CustomLookAndFeel.h"
#include "../Util/ValueTreeIdentifiers.h"

RootNode::RootNode() {
    nodeType = NodeType::Root;

    setPaintingIsUnclipped(true);

    rootRectangle = std::make_unique<RootRectangle>();
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

    // Rectangle sits at the left edge, slim, vertically centred, just touching the circle.
    // The circle fill starts at rw + 8px (due to 1.5 + 6.0 + 0.5 reductions in drawNode),
    // so extend the rectangle width to meet the circle's left visual edge.
    int rectHeight = bounds.getHeight() / 2;
    int rectY      = (bounds.getHeight() - rectHeight) / 2;
    rootRectangle->setBounds(0, rectY, rw + 8, rectHeight);

    // All node content (MIDI editor, buttons, count editor) lives in the circle area.
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
