//
// Created by Eli Baumgardner on 10/6/25.
//

#include "Modulator.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"

#include <algorithm>
#include <cmath>
#include <limits>

Modulator::Modulator(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::Modulator;
}

void Modulator::bindValueEditorForMode() {

    Node::bindValueEditorForMode();

    if (mode != NodeDisplayMode::Pitch || ! nodeValueTree.isValid()) {
        return;
    }

    auto offsetFormat = std::make_unique<NumberFormat>(minimumPitchOffset, maximumPitchOffset);
    offsetFormat->showsPositiveSign = true;

    nodeValueEditor.setFormat(std::move(offsetFormat));
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::ModAmount);
    nodeValueEditor.editable = true;
}

juce::Rectangle<float> Modulator::getSquareBounds() const {
    const auto  circleBounds = CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());
    const float side         = circleBounds.getWidth() * equalAreaSideFactor;

    return circleBounds.withSizeKeepingCentre(side, side);
}

float Modulator::getBodyExtent(juce::Point<float> approachDirection) const
{
    static constexpr float rayAxisEpsilon = 1.0e-4f;

    const juce::Rectangle<float> square = getSquareBounds();
    const juce::Point<float>     centre = (getNodeCentre() - getPosition()).toFloat();
    const juce::Point<float>     ray    = -approachDirection;

    float exit = std::numeric_limits<float>::max();

    if (std::abs(ray.x) > rayAxisEpsilon) {
        exit = std::min(exit, std::max((square.getX()     - centre.x) / ray.x,
                                       (square.getRight() - centre.x) / ray.x));
    }

    if (std::abs(ray.y) > rayAxisEpsilon) {
        exit = std::min(exit, std::max((square.getY()      - centre.y) / ray.y,
                                       (square.getBottom() - centre.y) / ray.y));
    }

    return std::max(0.0f, exit);
}

bool Modulator::hitTest(int x, int y) {
    const juce::Point<int> point(x, y);

    if (getSquareBounds().toNearestInt().contains(point)) {
        return true;
    }

    for (const ValueEditor* editor : { &countEditor, &switchCountEditor, &subLoopLimitEditor }) {
        if (editor->isVisible() && editor->getBounds().contains(point)) {
            return true;
        }
    }

    return false;
}

void Modulator::paint(juce::Graphics& g) {
    CustomLookAndFeel::get(*this).drawModulatorNode(g, getNodeVisual(getSquareBounds()));
}

