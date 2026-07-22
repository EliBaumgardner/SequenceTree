//
// Created by Eli Baumgardner on 7/20/26.
//

#include "NodeMenu.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include "Theme/CustomLookAndFeel.h"
#include "Node/Node.h"

NodeMenu::NodeMenu(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    countLimitEditor       .setMinimumValue(1);
    repeatEditor            .setMinimumValue(1);
    switchCountLimitEditor  .setMinimumValue(1);
    subLoopCountLimitEditor .setMinimumValue(1);
    velocityEditor          .setMinimumValue(0);
    pitchEditor              .setMinimumValue(0);
    channelEditor            .setMinimumValue(1);

    countLimitEditor       .setTooltip("Count Limit");
    repeatEditor            .setTooltip("Repeat Value");
    switchCountLimitEditor  .setTooltip("Switch Count Limit");
    subLoopCountLimitEditor .setTooltip("Sub Loop Count Limit");
    velocityEditor           .setTooltip("Velocity");
    pitchEditor               .setTooltip("Pitch");
    channelEditor             .setTooltip("Channel");

    colourLabel             .setText("COL", juce::dontSendNotification);
    countLimitLabel        .setText("CNT", juce::dontSendNotification);
    repeatLabel             .setText("RPT", juce::dontSendNotification);
    switchCountLimitLabel   .setText("SW",  juce::dontSendNotification);
    subLoopCountLimitLabel  .setText("SUB", juce::dontSendNotification);
    velocityLabel            .setText("VEL", juce::dontSendNotification);
    pitchLabel                .setText("PIT", juce::dontSendNotification);
    channelLabel              .setText("CH",  juce::dontSendNotification);

    for (auto& row : labeledRows) {
        row.label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        row.label.setFont(juce::Font(juce::FontOptions(9.0f)));
        row.label.setJustificationType(juce::Justification::centredLeft);
        addChildComponent(row.label);
        addChildComponent(row.editor);
    }

    colourLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    colourLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    colourLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(colourLabel);
    addAndMakeVisible(colourSelector);


    editTraversalRulesButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTextButton(g, bounds, state);
        }, context.lookAndFeel);

    editTraversalRulesButton->setText("edit traversal rules");
    editTraversalRulesButton->onClick = [this]() { traversalRulesLauncher.show(); };

    addAndMakeVisible(editTraversalRulesButton.get());

    applicationContext.addNodeSelectedListener([this](Node* node, bool selected) {
        colourSelector.setNode(selected ? node : nullptr);

        if (selected && node != nullptr) {
            bindToNode(node);
        } else {
            clearBindings();
        }

        resized();
        repaint();
    });
}

NodeMenu::~NodeMenu() {
    setLookAndFeel(nullptr);
}

void NodeMenu::bindToNode(Node* node) {
    const bool hasNodeTree = node->nodeValueTree.isValid();
    const bool hasMidi     = node->midiNoteData.isValid();

    if (hasNodeTree) {
        countLimitEditor       .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::CountLimit);
        repeatEditor            .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::RepeatValue);
        switchCountLimitEditor  .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);
        subLoopCountLimitEditor .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::SubLoopCountLimit);
    }

    if (hasMidi) {
        velocityEditor.bindEditor(node->midiNoteData, ValueTreeIdentifiers::MidiVelocity);
        pitchEditor   .bindEditor(node->midiNoteData, ValueTreeIdentifiers::MidiPitch);
        channelEditor .bindEditor(node->midiNoteData, ValueTreeIdentifiers::MidiChannel);
    }

    countLimitLabel         .setVisible(hasNodeTree);
    countLimitEditor        .setVisible(hasNodeTree);
    repeatLabel              .setVisible(hasNodeTree);
    repeatEditor             .setVisible(hasNodeTree);
    switchCountLimitLabel    .setVisible(hasNodeTree);
    switchCountLimitEditor   .setVisible(hasNodeTree);
    subLoopCountLimitLabel   .setVisible(hasNodeTree);
    subLoopCountLimitEditor  .setVisible(hasNodeTree);
    velocityLabel             .setVisible(hasMidi);
    velocityEditor            .setVisible(hasMidi);
    pitchLabel                 .setVisible(hasMidi);
    pitchEditor                .setVisible(hasMidi);
    channelLabel                .setVisible(hasMidi);
    channelEditor               .setVisible(hasMidi);
}

void NodeMenu::clearBindings() {
    for (auto& row : labeledRows) {
        row.label.setVisible(false);
        row.editor.setVisible(false);
    }
}

void NodeMenu::paint(juce::Graphics& g) {
    const Theme& theme = CustomLookAndFeel::get(*this);
    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds().toFloat());
}

void NodeMenu::resized() {
    auto bounds = getLocalBounds().reduced(Theme::menuEdgeInset);

    editTraversalRulesButton->setBounds(bounds.removeFromBottom(Theme::textButtonHeight));

    auto colourRowBounds = bounds.removeFromTop(rowHeight);
    colourLabel.setBounds(colourRowBounds.removeFromLeft(colourRowBounds.getWidth() / 3));
    colourSelector.setBounds(colourRowBounds);
    bounds.removeFromTop(rowGap);

    for (auto& row : labeledRows) {
        if (!row.editor.isVisible())
            continue;

        auto rowBounds = bounds.removeFromTop(rowHeight);
        row.label.setBounds(rowBounds.removeFromLeft(rowBounds.getWidth() / 3));
        row.editor.setBounds(rowBounds);
        bounds.removeFromTop(rowGap);
    }
}
