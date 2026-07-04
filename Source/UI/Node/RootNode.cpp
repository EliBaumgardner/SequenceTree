//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "NodeTextEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"

RootNode::RootNode(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::Root;

    setPaintingIsUnclipped(true);

    rootRectangle = std::make_unique<RootRectangle>(context);
    addAndMakeVisible(rootRectangle.get());

    ValueEditor& traversalEditor = rootRectangle->loopLimitEditor;

    traversalEditor.boundValue.setValue(1);


    traversalEditor.onValueChange = [this, &traversalEditor]() {

        juce::String text = traversalEditor.boundValue.toString();

        std::vector<int> words;
        juce::String word;

        for (int i = 0; i < text.length(); i++) {
            char c = text[i];

            if (std::isdigit(c)) {
                word += c;
            }
            else {
                words.push_back(word.getIntValue());
                word.clear();
            }
        }

        for (int i = 0; i < words.size(); i++) {

            int traversalId = words[i];

            juce::ValueTree traversalData = ValueTreeState::traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

            if (!traversalData.isValid()) {
                ValueTreeState::createTraversalData(traversalId, nullptr);
            }

            juce::ValueTree traversalIdTree {ValueTreeIdentifiers::TraversalId};
            traversalIdTree.setProperty(ValueTreeIdentifiers::TraversalId, traversalId, nullptr);
            nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds).addChild(traversalIdTree, -1, nullptr);
        }
    };
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
    juce::Rectangle<int> editorArea = CustomLookAndFeel::getNodeCircleBounds(circleArea.toFloat()).toNearestInt().reduced(6);

    upButton.setBounds(editorArea.removeFromTop(4));
    downButton.setBounds(editorArea.removeFromBottom(4));

    nodeTextEditor->setBounds(editorArea);
    nodeTextEditor->setJustification(juce::Justification::centred);

    countEditor.setBounds(circleArea.getRight() - 18, circleArea.getY(), 18, 12);
    switchCountEditor.setBounds( circleArea.getRight() - 18, circleArea.getBottom() - 12, 18, 12);
    subLoopLimitEditor.setBounds(circleArea.getBottomLeft().getX(), circleArea.getBottom() - 12, 18, 12);
}

