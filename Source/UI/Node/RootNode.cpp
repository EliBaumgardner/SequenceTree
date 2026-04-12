//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "NodeTextEditor.h"
#include "CustomLookAndFeel.h"

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
    juce::Rectangle<int> bounds = getLocalBounds();

    float rootRectangleWidth = 8.0f;
    float rootRectangleHeight = 5.0f;

    float editorAreaSize = 10.0f;
    float upButtonSize = 4.0f;
    float downButtonSize = 4.0f;

    juce::Rectangle<int> rootRectangleBounds = bounds.withTrimmedTop(rootRectangleHeight);
    rootRectangleBounds = rootRectangleBounds.withTrimmedBottom(rootRectangleHeight);

    rootRectangle->setBounds(rootRectangleBounds.removeFromLeft(rootRectangleWidth));

    juce::Rectangle<int> editorArea = bounds.reduced(editorAreaSize);

    upButton.setBounds(editorArea.removeFromTop(upButtonSize));
    downButton.setBounds(editorArea.removeFromBottom(downButtonSize));

    nodeTextEditor->setBounds(editorArea);
    nodeTextEditor->setJustification(juce::Justification::centred);
}
