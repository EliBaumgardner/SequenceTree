//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Util/ApplicationContext.h"

#include <algorithm>

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

void RootNode::bindToTree()
{
    Node::bindToTree();

    if (! nodeValueTree.isValid()) {
        return;
    }

    const juce::ValueTree traversalChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    juce::StringArray equippedReferences;

    for (int i = 0; i < traversalChildrenIds.getNumChildren(); i++) {
        const juce::ValueTree reference = traversalChildrenIds.getChild(i);

        const TraversalKey key { (int) reference.getProperty(ValueTreeIdentifiers::TraversalId),
                                 (int) reference.getProperty(ValueTreeIdentifiers::TraversalInstance, 0) };

        equippedReferences.add(TraversalRefListFormat::describe(key));
    }

    rootRectangle->traversalEditor.setText(equippedReferences.joinIntoString(" "));
}

RootNode::~RootNode() = default;

void RootNode::equipTraversals()
{
    if (! nodeValueTree.isValid()) {
        return;
    }

    const std::vector<TraversalKey> keys = TraversalRefListFormat::parse(rootRectangle->traversalEditor.getText());

    juce::ValueTree traversalChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    for (int i = traversalChildrenIds.getNumChildren() - 1; i >= 0; i--) {

        const juce::ValueTree reference = traversalChildrenIds.getChild(i);

        const TraversalKey existingKey { (int) reference.getProperty(ValueTreeIdentifiers::TraversalId),
                                         (int) reference.getProperty(ValueTreeIdentifiers::TraversalInstance, 0) };

        if (std::find(keys.begin(), keys.end(), existingKey) == keys.end()) {
            traversalChildrenIds.removeChild(i, nullptr);
        }
    }

    for (const TraversalKey& key : keys) {

        applicationContext.graphState->traversals.addTraversalData(key.typeId, nullptr);

        if (TraversalState::findReference(traversalChildrenIds, key).isValid()) {
            continue;
        }

        juce::ValueTree traversalIdTree {ValueTreeIdentifiers::TraversalId};
        traversalIdTree.setProperty(ValueTreeIdentifiers::TraversalId,       key.typeId,   nullptr);
        traversalIdTree.setProperty(ValueTreeIdentifiers::TraversalInstance, key.instance, nullptr);
        traversalChildrenIds.addChild(traversalIdTree, -1, nullptr);
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

