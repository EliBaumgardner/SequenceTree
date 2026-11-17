#include "Modulator.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <cmath>
#include <limits>

Modulator::Modulator(juce::UndoManager& undoManager) : Node(undoManager)
{
    nodeType = NodeInfo::Type::Modulator;

    setTooltip("Modulator");
}

void Modulator::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::NodeVisual visual = getNodeVisual(getSquareBounds());

    ArsNovaUI::CustomLookAndFeel::get(*this).drawSquareNode(graphics, visual);
}

void Modulator::bindValueEditorForMode()
{
    Node::bindValueEditorForMode();

    if (mode != NodeInfo::DisplayMode::Pitch || ! nodeValueTree.isValid()) {
        return;
    }

    auto offsetFormat = std::make_unique<ArsNovaUI::NumberFormat>(minimumPitchOffset, maximumPitchOffset);

    offsetFormat->showsPositiveSign = true;

    nodeValueEditor.setFormat(std::move(offsetFormat));
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::ModAmount);
    nodeValueEditor.setTooltip("Pitch Offset");
    nodeValueEditor.editable = true;
}

bool Modulator::hitTest(int x, int y)
{
    const juce::Point<int> point(x, y);

    if (getSquareBounds().toNearestInt().contains(point)) {
        return true;
    }

    for (const ArsNovaUI::ValueEditor* editor : { &countEditor, &switchCountEditor, &subLoopLimitEditor }) {
        if (editor->isVisible() && editor->getBounds().contains(point)) {
            return true;
        }
    }

    return false;
}

float Modulator::getBodyExtent(juce::Point<float> approachDirection) const
{
    static constexpr float rayAxisEpsilon = 1.0e-4f;

    const juce::Rectangle<float> square = getSquareBounds();
    const juce::Point<float>     centre = (getNodeCentre() - getPosition()).toFloat();
    const juce::Point<float>     ray    = -approachDirection;
    float                        exit   = std::numeric_limits<float>::max();

    if (std::abs(ray.x) > rayAxisEpsilon) {
        const float toLeft      = (square.getX()     - centre.x) / ray.x;
        const float toRight     = (square.getRight() - centre.x) / ray.x;
        const float fartherSide = std::max(toLeft, toRight);

        exit = std::min(exit, fartherSide);
    }

    if (std::abs(ray.y) > rayAxisEpsilon) {
        const float toTop       = (square.getY()      - centre.y) / ray.y;
        const float toBottom    = (square.getBottom() - centre.y) / ray.y;
        const float fartherSide = std::max(toTop, toBottom);

        exit = std::min(exit, fartherSide);
    }

    return std::max(0.0f, exit);
}

juce::Rectangle<float> Modulator::getSquareBounds() const
{
    const auto  circleBounds = ArsNovaUI::CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());
    const float side         = circleBounds.getWidth() * equalAreaSideFactor;

    return circleBounds.withSizeKeepingCentre(side, side);
}
