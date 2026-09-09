//
// Created by Eli Baumgardner on 10/6/25.
//

#include "Modulator.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"

Modulator::Modulator(ApplicationContext& context) : Node(context)
{
    nodeType = NodeType::Modulator;
}

void Modulator::bindValueEditorForMode() {

    if (mode != NodeDisplayMode::Pitch) {
        nodeValueEditor.disableSignedValue();
        Node::bindValueEditorForMode();
        return;
    }

    Node::bindValueEditorForMode();

    if (! nodeValueTree.isValid()) {
        return;
    }

    nodeValueEditor.setPitchMode(false);
    nodeValueEditor.enableSignedValue(minimumPitchOffset, maximumPitchOffset);
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::ModAmount);
}

juce::Rectangle<float> Modulator::getSquareBounds() const {
    const auto  circleBounds = CustomLookAndFeel::getNodeCircleBounds(getLocalBounds().toFloat());
    const float side         = circleBounds.getWidth() * equalAreaSideFactor;

    return circleBounds.withSizeKeepingCentre(side, side);
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

