#include "Modulator.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include <cmath>
#include <limits>

Modulator::Modulator(juce::UndoManager& undoManager) : Node(undoManager)
{
    nodeType = NodeInfo::Type::Modulator;
}

void Modulator::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawSquareNode(graphics, getNodeVisual(getSquareBounds()));
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
        exit = std::min(exit, std::max((square.getX() - centre.x) / ray.x, (square.getRight() - centre.x) / ray.x));
    }

    if (std::abs(ray.y) > rayAxisEpsilon) {
        exit = std::min(exit, std::max((square.getY() - centre.y) / ray.y, (square.getBottom() - centre.y) / ray.y));
    }

    return std::max(0.0f, exit);
}

juce::Rectangle<float> Modulator::getSquareBounds() const
{
    const auto  circleBounds = ArsNovaUI::CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());
    const float side         = circleBounds.getWidth() * equalAreaSideFactor;

    return circleBounds.withSizeKeepingCentre(side, side);
}
