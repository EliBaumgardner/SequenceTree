#include "OrnamentCanvas.h"
#include "../Canvas/CanvasHitTester.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/OrnamentLibraries.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/ArrowInfo.h"

#include <algorithm>

OrnamentCanvas::OrnamentCanvas(juce::UndoManager& undoManager)
    : undoManager(undoManager)
{
}

void OrnamentCanvas::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme     = ArsNovaUI::CustomLookAndFeel::get(*this);
    const float             grid      = ArrowInfo::pixelsPerGridSpace;
    const auto              visible   = getLocalBounds().toFloat().transformedBy(modelTransform);
    const float             lineWidth = 1.0f / viewTransform.getScaleFactor();

    graphics.setColour(theme.canvasColour);
    graphics.fillRect(getLocalBounds());

    graphics.addTransform(viewTransform);

    graphics.setColour(theme.gridColour);

    for (float x = std::floor(visible.getX() / grid) * grid; x < visible.getRight(); x += grid) {
        graphics.drawLine(x, visible.getY(), x, visible.getBottom(), lineWidth);
    }

    for (float y = std::floor(visible.getY() / grid) * grid; y < visible.getBottom(); y += grid) {
        graphics.drawLine(visible.getX(), y, visible.getRight(), y, lineWidth);
    }

    graphics.setColour(theme.borderStrongColour);
    graphics.drawLine(visible.getX(), 0.0f, visible.getRight(), 0.0f, lineWidth);
}

void OrnamentCanvas::childrenChanged()
{
    for (juce::Component* child : getChildren()) {
        child->setTransform(viewTransform);
    }
}

void OrnamentCanvas::handleAsyncUpdate()
{
    const int          diameter   = ArsNovaUI::Theme::nodeRadius * 2;
    const juce::Colour nodeColour = ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour;
    const int          hostId     = figure.getProperty(ValueTreeIdentifiers::OrnamentHostId, OrnamentLibraries::noHostId);

    for (int arrowIndex = arrows.size() - 1; arrowIndex >= 0; --arrowIndex) {
        const Arrow* const    arrow      = arrows[arrowIndex];
        const juce::ValueTree startTree  = arrow->startNode->nodeValueTree;
        const bool            startStays = startTree.getParent() == figure && arrow->arrowTree.getParent().getParent() == startTree;
        bool                  endStays   = arrow->isDangling();

        if (! arrow->isDangling()) {
            endStays = arrow->endNode->nodeValueTree.getParent() == figure
                    && arrow->endNode->nodeId == static_cast<int>(arrow->arrowTree.getProperty(ValueTreeIdentifiers::Id, -1));
        }

        if (! startStays || ! endStays) {
            arrows.remove(arrowIndex);
        }
    }

    std::erase_if(nodes, [this](const auto& entry) { return entry.second->nodeValueTree.getParent() != figure; });

    for (const juce::ValueTree& nodeTree : figure) {
        const int              nodeId = nodeTree.getProperty(ValueTreeIdentifiers::Id);
        const juce::Point<int> centre { nodeTree.getProperty(ValueTreeIdentifiers::XPosition), nodeTree.getProperty(ValueTreeIdentifiers::YPosition) };
        auto&                  node   = nodes[nodeId];

        if (node == nullptr) {
            node = std::make_unique<OrnamentNode>(undoManager);

            node->nodeId        = nodeId;
            node->nodeValueTree = nodeTree;
            node->nodeColour    = nodeColour;

            node->bindToTree();
            node->setDisplayMode(NodeDisplayMode::Pitch);
            node->setMouseCursor(juce::MouseCursor::ParentCursor);

            addAndMakeVisible(*node);
        }

        if (node->hasInnerRim != (nodeId == hostId)) {
            node->hasInnerRim = nodeId == hostId;

            node->repaint();
        }

        node->setBounds(juce::Rectangle<int>(diameter, diameter).withCentre(centre));
    }

    updateArrows();
}

OrnamentNode* OrnamentCanvas::findNodeAt(juce::Point<int> point) const
{
    for (const auto& [nodeId, node] : nodes) {
        if (node->getBounds().contains(point)) {
            return node.get();
        }
    }

    return nullptr;
}

Arrow* OrnamentCanvas::findArrowNear(juce::Point<float> point, float radius, ArrowPart part) const
{
    Arrow* nearest         = nullptr;
    float  closestDistance = radius;

    for (Arrow* const arrow : arrows) {
        float distance = radius;

        switch (part) {
            case ArrowPart::Shaft:
                distance = CanvasHitTester::distanceToSegment(point, arrow->startNode->getNodeCentre().toFloat(), arrow->getTip().toFloat());
                break;

            case ArrowPart::Head:
                if (! arrow->isDangling()) {
                    distance = point.getDistanceFrom(arrow->getHeadAnchor());
                }
                break;

            case ArrowPart::Label:
                if (arrow->showsDurationLabel()) {
                    distance = point.getDistanceFrom(arrow->getLabel(arrow->getGeometry(1.0f), Arrow::arrowHeadLength).centre);
                }
                break;

            case ArrowPart::DanglingTip:
                if (arrow->isDangling()) {
                    distance = point.getDistanceFrom(arrow->getTip().toFloat());
                }
                break;
        }

        if (distance < closestDistance) {
            closestDistance = distance;
            nearest         = arrow;
        }
    }

    return nearest;
}

void OrnamentCanvas::updateArrows()
{
    for (const auto& [nodeId, node] : nodes) {
        const juce::ValueTree connection         = node->nodeValueTree.getChildWithName(ValueTreeIdentifiers::NodeChildrenIds).getChild(0);
        const juce::ValueTree dangling           = node->nodeValueTree.getChildWithName(ValueTreeIdentifiers::DanglingArrows).getChild(0);
        const auto            child              = nodes.find(static_cast<int>(connection.getProperty(ValueTreeIdentifiers::Id, -1)));
        const bool            hasConnectionArrow = std::any_of(arrows.begin(), arrows.end(), [&connection](const Arrow* arrow) { return arrow->arrowTree == connection; });
        const bool            hasDanglingArrow   = std::any_of(arrows.begin(), arrows.end(), [&dangling](const Arrow* arrow) { return arrow->arrowTree == dangling; });

        if (connection.isValid() && child != nodes.end() && ! hasConnectionArrow) {
            Arrow* const arrow = arrows.add(std::make_unique<Arrow>(node.get(), child->second.get(), undoManager));

            arrow->arrowTree = connection;

            arrow->setInterceptsMouseClicks(false, true);

            addAndMakeVisible(*arrow);

            arrow->toBack();
        }

        if (dangling.isValid() && ! hasDanglingArrow) {
            Arrow* const arrow = arrows.add(std::make_unique<Arrow>(node.get(), juce::Point<int> {}, undoManager));

            arrow->arrowTree = dangling;

            arrow->valueEditor->setVisible(false);

            addAndMakeVisible(*arrow);

            arrow->toBack();
        }
    }

    for (Arrow* const arrow : arrows) {
        if (arrow->isDangling()) {
            arrow->setTipOffset({ arrow->arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipX), arrow->arrowTree.getProperty(ValueTreeIdentifiers::ArrowTipY) });
        }
        else {
            arrow->setArrowBounds();
        }
    }
}
