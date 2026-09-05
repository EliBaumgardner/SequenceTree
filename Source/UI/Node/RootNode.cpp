//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
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

    rootRectangle->traversalEditor.onValueChange = [this]() {
        equipTraversals();
    };
}

void RootNode::setDisplayMode(NodeDisplayMode mode)
{
    Node::setDisplayMode(mode);

    if (! nodeValueTree.isValid()) {
        return;
    }

    const juce::ValueTree traversalChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    juce::StringArray equippedIds;

    for (int i = 0; i < traversalChildrenIds.getNumChildren(); i++) {
        equippedIds.add(traversalChildrenIds.getChild(i).getProperty(ValueTreeIdentifiers::TraversalId).toString());
    }

    rootRectangle->traversalEditor.setText(equippedIds.joinIntoString(" "));
}

RootNode::~RootNode() = default;

void RootNode::equipTraversals()
{
    if (! nodeValueTree.isValid()) {
        return;
    }

    const std::vector<int> words = IntListFormat::parse(rootRectangle->traversalEditor.getText());

    auto contains = [&words](int id) {
        for (int w : words) {
            if (w == id) {
                return true;
            }
        }
        return false;
    };

    juce::ValueTree traversalChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    for (int i = traversalChildrenIds.getNumChildren() - 1; i >= 0; i--) {

        const int existingId = traversalChildrenIds.getChild(i).getProperty(ValueTreeIdentifiers::TraversalId);

        if (!contains(existingId)) {
            traversalChildrenIds.removeChild(i, nullptr);
        }
    }

    for (const int traversalId : words) {

        if (!applicationContext.valueTreeState->traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {
            applicationContext.valueTreeState->createTraversalData(traversalId, nullptr);
        }

        if (!traversalChildrenIds.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId).isValid()) {

            juce::ValueTree traversalIdTree {ValueTreeIdentifiers::TraversalId};
            traversalIdTree.setProperty(ValueTreeIdentifiers::TraversalId, traversalId, nullptr);
            traversalChildrenIds.addChild(traversalIdTree, -1, nullptr);
        }
    }

    applicationContext.rtGraphBuilder->makeRTGraph(nodeValueTree);
}

void RootNode::paint(juce::Graphics& g)
{
    const auto circleBounds = getLocalBounds().toFloat()
                            .withTrimmedLeft((float) loopLimitRectangleWidth);

    CustomLookAndFeel::get(*this).drawNode(g, getNodeVisual(circleBounds));
}

void RootNode::resized() {
    const int rw = loopLimitRectangleWidth;

    const juce::Rectangle<int> bounds = getLocalBounds();

    const int rectHeight = bounds.getHeight() / 2;
    const int rectY      = (bounds.getHeight() - rectHeight) / 2;

    rootRectangle->setBounds(0, rectY, rw + 8, rectHeight);

    layoutInterior(bounds.withTrimmedLeft(rw));
}

