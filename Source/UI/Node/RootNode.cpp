//
// Created by Eli Baumgardner on 4/11/26.
//

#include "RootNode.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Editors/Formats/ValueFormat.h"
#include "../../Util/ApplicationContext.h"

#include <algorithm>
#include <cmath>
#include <limits>

RootNode::RootNode(const ApplicationContext& context) : Node(context)
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

        const TraversalKey key { static_cast<int>(reference.getProperty(ValueTreeIdentifiers::TraversalId)),
                                 static_cast<int>(reference.getProperty(ValueTreeIdentifiers::TraversalInstance, 0)) };

        equippedReferences.add(TraversalFlagFormat::describe(key));
    }

    rootRectangle->traversalEditor.commitText(equippedReferences.joinIntoString(" "));
}

RootNode::~RootNode() = default;

void RootNode::equipTraversals()
{
    if (! nodeValueTree.isValid()) {
        return;
    }

    ValueEditor& traversalEditor = rootRectangle->traversalEditor;

    const std::vector<TraversalKey> keys =
        TraversalFlagFormat::parseKeys(traversalEditor.boundValue.getValue().toString());

    juce::ValueTree traversalChildrenIds = nodeValueTree.getChildWithName(ValueTreeIdentifiers::TraversalChildrenIds);

    for (int i = traversalChildrenIds.getNumChildren() - 1; i >= 0; i--) {

        const juce::ValueTree reference = traversalChildrenIds.getChild(i);

        const TraversalKey existingKey { static_cast<int>(reference.getProperty(ValueTreeIdentifiers::TraversalId)),
                                         static_cast<int>(reference.getProperty(ValueTreeIdentifiers::TraversalInstance, 0)) };

        if (std::ranges::find(keys, existingKey) == keys.end()) {
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
}

float RootNode::getBodyExtent(juce::Point<float> approachDirection) const
{
    static constexpr float rayAxisEpsilon = 1.0e-4f;

    const juce::Rectangle<float> circle = CustomLookAndFeel::getNodeCircleBounds(
        getLocalBounds().toFloat().withTrimmedLeft(static_cast<float>(loopLimitRectangleWidth)));

    const juce::Point<float> centre   = (getNodeCentre() - getPosition()).toFloat();
    const juce::Point<float> toCircle = circle.getCentre() - centre;

    const float radius    = std::max(0.0f, circle.getWidth() * 0.5f);
    const float along     = approachDirection.getDotProduct(toCircle);
    const float clearance = along * along - toCircle.getDistanceSquaredFromOrigin() + radius * radius;

    float circleExtent = radius;

    if (clearance > 0.0f) {
        circleExtent = std::max(0.0f, std::sqrt(clearance) - along);
    }

    const juce::Rectangle<float> rectangle = rootRectangle->getBounds().toFloat();
    const juce::Point<float>     ray       = -approachDirection;

    float entry = 0.0f;
    float exit  = std::numeric_limits<float>::max();

    if (std::abs(ray.x) < rayAxisEpsilon) {
        if (centre.x < rectangle.getX() || centre.x > rectangle.getRight()) {
            return circleExtent;
        }
    }
    else {
        const float toLeft  = (rectangle.getX()     - centre.x) / ray.x;
        const float toRight = (rectangle.getRight() - centre.x) / ray.x;

        entry = std::max(entry, std::min(toLeft, toRight));
        exit  = std::min(exit,  std::max(toLeft, toRight));
    }

    if (std::abs(ray.y) < rayAxisEpsilon) {
        if (centre.y < rectangle.getY() || centre.y > rectangle.getBottom()) {
            return circleExtent;
        }
    }
    else {
        const float toTop    = (rectangle.getY()      - centre.y) / ray.y;
        const float toBottom = (rectangle.getBottom() - centre.y) / ray.y;

        entry = std::max(entry, std::min(toTop, toBottom));
        exit  = std::min(exit,  std::max(toTop, toBottom));
    }

    if (exit < entry) {
        return circleExtent;
    }

    return std::max(circleExtent, exit);
}

void RootNode::paint(juce::Graphics& g)
{
    const auto circleBounds = getLocalBounds().toFloat()
                            .withTrimmedLeft(static_cast<float>(loopLimitRectangleWidth));

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

