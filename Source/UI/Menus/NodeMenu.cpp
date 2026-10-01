//
// Created by Eli Baumgardner on 7/20/26.
//

#include "NodeMenu.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Canvas/NodeCanvas.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Node/Node.h"

NodeMenu::NodeMenu(const ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    auto repeatFormat = std::make_unique<NumberFormat>(minimumRepeatValue, maximumRepeatValue);
    repeatFormat->prefix = "x";
    auto probabilityFormat = std::make_unique<NumberFormat>(minimumProbability, maximumProbability);
    probabilityFormat->suffix = "%";
    countLimitEditor        .setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));
    repeatEditor            .setFormat(std::move(repeatFormat));
    switchCountLimitEditor  .setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));
    subLoopCountLimitEditor .setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));
    probabilityEditor       .setFormat(std::move(probabilityFormat));
    velocityEditor          .setFormat(std::make_unique<NumberFormat>(minimumMidiVelocity, maximumMidiVelocity));
    pitchEditor             .setFormat(std::make_unique<NumberFormat>(minimumMidiPitch, maximumMidiPitch));
    channelEditor           .setFormat(std::make_unique<NumberFormat>(minimumMidiChannel, maximumMidiChannel));

    countLimitEditor       .setTooltip("Count Limit");
    repeatEditor            .setTooltip("Repeat Value");
    switchCountLimitEditor  .setTooltip("Switch Count Limit");
    subLoopCountLimitEditor .setTooltip("Sub Loop Count Limit");
    probabilityEditor       .setTooltip("Probability");
    velocityEditor           .setTooltip("Velocity");
    pitchEditor               .setTooltip("Pitch");
    channelEditor             .setTooltip("Channel");

    colourLabel             .setText("COL", juce::dontSendNotification);
    countLimitLabel        .setText("CNT", juce::dontSendNotification);
    repeatLabel             .setText("RPT", juce::dontSendNotification);
    switchCountLimitLabel   .setText("SW",  juce::dontSendNotification);
    subLoopCountLimitLabel  .setText("SUB", juce::dontSendNotification);
    probabilityLabel        .setText("PRB", juce::dontSendNotification);
    velocityLabel            .setText("VEL", juce::dontSendNotification);
    pitchLabel                .setText("PIT", juce::dontSendNotification);
    channelLabel              .setText("CH",  juce::dontSendNotification);

    for (juce::Label* label : { &colourLabel, &countLimitLabel, &repeatLabel, &switchCountLimitLabel, &subLoopCountLimitLabel,
                                &probabilityLabel, &velocityLabel, &pitchLabel, &channelLabel }) {
        label->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label->setMinimumHorizontalScale(1.0f);
        label->setBorderSize({});
        label->setJustificationType(juce::Justification::centredLeft);
    }

    for (const auto& row : labeledRows) {
        addChildComponent(row.label);
        addChildComponent(row.editor);
    }

    addAndMakeVisible(colourLabel);
    addAndMakeVisible(colourSelector);

    editTraversalRulesButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTextButton(g, bounds, state, CustomLookAndFeel::get(*this).textHeight);
        }, context.lookAndFeel);

    editTraversalRulesButton->setText("edit traversal rules");
    editTraversalRulesButton->onClick = [this]() { traversalRulesLauncher.show(); };

    addAndMakeVisible(editTraversalRulesButton.get());

    applicationContext.canvas->nodeManager.nodeSelectedListeners.push_back([this](Node* node, bool selected) {
        if (selected) {
            colourSelector.setNode(node);
        } else {
            colourSelector.setNode(nullptr);
        }

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

void NodeMenu::bindToNode(const Node* node) {
    const bool hasNodeTree = node->nodeValueTree.isValid();
    const bool hasMidi     = node->midiNoteData.isValid();
    const bool hasSubLoop  = hasNodeTree && ! node->isAlternativeNode;

    if (hasNodeTree) {
        countLimitEditor       .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::CountLimit);
        repeatEditor            .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::RepeatValue);
        switchCountLimitEditor  .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::SwitchCountLimit);
        probabilityEditor       .bindEditor(node->nodeValueTree, ValueTreeIdentifiers::Probability);
    }

    if (hasSubLoop) {
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
    subLoopCountLimitLabel   .setVisible(hasSubLoop);
    subLoopCountLimitEditor  .setVisible(hasSubLoop);
    probabilityLabel         .setVisible(hasNodeTree);
    probabilityEditor        .setVisible(hasNodeTree);
    velocityLabel             .setVisible(hasMidi);
    velocityEditor            .setVisible(hasMidi);
    pitchLabel                 .setVisible(hasMidi);
    pitchEditor                .setVisible(hasMidi);
    channelLabel                .setVisible(hasMidi);
    channelEditor               .setVisible(hasMidi);
}

void NodeMenu::clearBindings() {
    for (const auto& row : labeledRows) {
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
    int        barHeight  = static_cast<int>(getHeight() * Theme::barHeightRatio);
    int        spacing    = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    int        rowHeight  = juce::roundToInt(barHeight * Theme::menuRowHeightRatio);
    auto       bounds     = getLocalBounds().reduced(spacing);
    float      textHeight = CustomLookAndFeel::get(*this).textHeight;
    juce::Font textFont   { juce::FontOptions(textHeight) };

    editTraversalRulesButton->setBounds(bounds.removeFromBottom(juce::roundToInt(barHeight * Theme::menuButtonHeightRatio)));

    auto colourRowBounds = bounds.removeFromTop(rowHeight);
    colourLabel.setFont(textFont);
    colourLabel.setBounds(colourRowBounds.removeFromLeft(colourRowBounds.getWidth() / 3));
    colourSelector.setBounds(colourRowBounds);
    bounds.removeFromTop(spacing);

    for (const auto& row : labeledRows) {
        row.label.setFont(textFont);
        row.editor.setFontHeight(textHeight);

        if (!row.editor.isVisible()) {
            continue;
        }

        auto rowBounds = bounds.removeFromTop(rowHeight);
        row.label.setBounds(rowBounds.removeFromLeft(rowBounds.getWidth() / 3));
        row.editor.setBounds(rowBounds);
        bounds.removeFromTop(spacing);
    }
}
