#include "BottomBar.h"
#include "Theme/CustomLookAndFeel.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "Node/Node.h"

BottomBar::BottomBar(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    addAndMakeVisible(colourSelector);

    countLimitEditor.setMinimumValue(1);
    repeatEditor    .setMinimumValue(1);
    velocityEditor  .setMinimumValue(0);
    pitchEditor     .setMinimumValue(0);
    channelEditor   .setMinimumValue(1);

    countLimitEditor.setTooltip("Count Limit");
    repeatEditor    .setTooltip("Repeat Value");
    velocityEditor  .setTooltip("Velocity");
    pitchEditor     .setTooltip("Pitch");
    channelEditor   .setTooltip("Channel");

    for (auto& le : labeledEditors) {
        le.editor.setVisible(false);
        addChildComponent(le.editor);
    }

    applicationContext.onNodeSelected = [this](Node* node, bool selected) {
        colourSelector.setNode(selected ? node : nullptr);

        if (selected && node != nullptr) {
            bindToNode(node);
        } else {
            clearBindings();
        }

        resized();
        repaint();
    };
}

void BottomBar::bindToNode(Node* node)
{
    const bool hasNodeTree = node->nodeValueTree.isValid();
    const bool hasMidi     = node->midiNoteData.isValid();

    if (hasNodeTree) {
        countLimitEditor.bindEditor(node->nodeValueTree, ValueTreeIdentifiers::CountLimit);
        repeatEditor    .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::RepeatValue);
    }

    if (hasMidi) {
        velocityEditor.bindEditor(node->midiNoteData, ValueTreeIdentifiers::MidiVelocity);
        pitchEditor   .bindEditor(node->midiNoteData, ValueTreeIdentifiers::MidiPitch);
        channelEditor .bindEditor(node->midiNoteData, ValueTreeIdentifiers::MidiChannel);
    }

    countLimitEditor.setVisible(hasNodeTree);
    repeatEditor    .setVisible(hasNodeTree);
    velocityEditor  .setVisible(hasMidi);
    pitchEditor     .setVisible(hasMidi);
    channelEditor   .setVisible(hasMidi);
}

void BottomBar::clearBindings()
{
    for (auto& le : labeledEditors) {
        le.editor.setVisible(false);
    }
}

void BottomBar::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawBottomBar(g, *this);

    g.setFont(juce::Font(juce::FontOptions(9.0f)));
    g.setColour(juce::Colours::lightgrey.withAlpha(0.7f));

    for (const auto& labeledEditor : labeledEditors) {
        if (!labeledEditor.editor.isVisible())
            continue;

        auto editorBounds = labeledEditor.editor.getBounds();
        auto labelBounds  = editorBounds.withX(editorBounds.getX() - labelWidth).withWidth(labelWidth);

        g.drawText(labeledEditor.label, labelBounds, juce::Justification::centredRight, false);
    }
}

void BottomBar::resized()
{
    auto bounds= getLocalBounds().reduced(4);
    int  height               = bounds.getHeight();

    colourSelector.setBounds(bounds.removeFromLeft(height));
    bounds.removeFromLeft(16);

    for (auto& le : labeledEditors) {
        if (!le.editor.isVisible())
            continue;

        bounds.removeFromLeft(labelWidth);
        le.editor.setBounds(bounds.removeFromLeft(editorWidth));
        bounds.removeFromLeft(cellGap);
    }
}
