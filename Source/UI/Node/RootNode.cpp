//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "NodeTextEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Util/ApplicationContext.h"

RootNode::RootNode(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::Root;

    setPaintingIsUnclipped(true);

    rootRectangle = std::make_unique<RootRectangle>(context);
    addAndMakeVisible(rootRectangle.get());

    subLoopLimitEditor.setTooltip("Loop Limit");

    ValueEditor& traversalEditor = rootRectangle->loopLimitEditor;

    traversalEditor.boundValue.setValue(1);


    traversalEditor.onValueChange = [this, &traversalEditor]() {

        DBG("traversal chhanged");

        juce::String text = traversalEditor.boundValue.toString();

        std::vector<int> words;
        juce::String word;

        for (int i = 0; i <= text.length(); i++) {

            bool isDigit = i < text.length() && juce::CharacterFunctions::isDigit(text[i]);

            if (isDigit) {
                word += text[i];
            }
            else if (word.isNotEmpty()) {
                words.push_back(word.getIntValue());
                word.clear();
            }
        }

        auto contains = [&words](int id) {
            for (int w : words) {
                if (w == id) return true;
            }
            return false;
        };

        juce::ValueTree traversalChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

        for (int i = traversalChildrenIds.getNumChildren() - 1; i >= 0; i--) {

            int existingId = traversalChildrenIds.getChild(i).getProperty(ValueTreeIdentifiers::TraversalId);

            if (!contains(existingId)) {
                traversalChildrenIds.removeChild(i, nullptr);
            }
        }

        for (int traversalId : words) {

            if (!ValueTreeState::traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {
                ValueTreeState::createTraversalData(traversalId, nullptr);
            }

            if (!traversalChildrenIds.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {

                juce::ValueTree traversalIdTree {ValueTreeIdentifiers::TraversalId};
                traversalIdTree.setProperty(ValueTreeIdentifiers::TraversalId, traversalId, nullptr);
                traversalChildrenIds.addChild(traversalIdTree, -1, nullptr);
            }
        }

        applicationContext.rtGraphBuilder->makeRTGraph(nodeValueTree);
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

