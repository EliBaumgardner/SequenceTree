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

void Modulator::setDisplayMode(NodeDisplayMode mode) {

    if (mode != NodeDisplayMode::Pitch) {
        nodeValueEditor.disableSignedValue();
        Node::setDisplayMode(mode);
        return;
    }

    Node::setDisplayMode(mode);

    if (! nodeValueTree.isValid()) {
        return;
    }

    nodeValueEditor.setPitchMode(false);
    nodeValueEditor.enableSignedValue(minimumPitchOffset, maximumPitchOffset);
    nodeValueEditor.bindEditor(nodeValueTree, ValueTreeIdentifiers::ModAmount);
    nodeValueEditor.repaint();
}

juce::Rectangle<float> Modulator::getSquareBounds() const {
    const float side = (float) juce::jmin(getWidth(), getHeight());

    return getLocalBounds().toFloat().withSizeKeepingCentre(side, side);
}

float Modulator::getVisualRadius() const {
    return getSquareBounds().getWidth() * 0.5f;
}

void Modulator::resized() {
    const auto square = getSquareBounds().toNearestInt();

    auto      editorArea   = square.reduced(editorAreaBoundsReduction);
    const int buttonHeight = juce::jmax(2, (int)(editorArea.getHeight() * 0.2f));

    upButton->setBounds(editorArea.removeFromTop(buttonHeight));
    downButton->setBounds(editorArea.removeFromBottom(buttonHeight));

    nodeValueEditor.setBounds(editorArea);

    const int editorWidth  = juce::roundToInt(square.getWidth()  * cornerEditorWidthFactor);
    const int editorHeight = juce::roundToInt(square.getHeight() * cornerEditorHeightFactor);

    countEditor.setBounds(square.getRight() - editorWidth, square.getY() - editorHeight,
                          editorWidth, editorHeight);

    switchCountEditor.setBounds(square.getRight() - editorWidth, square.getBottom(),
                                editorWidth, editorHeight);

    subLoopLimitEditor.setBounds(square.getX(), square.getBottom(),
                                 editorWidth, editorHeight);
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

