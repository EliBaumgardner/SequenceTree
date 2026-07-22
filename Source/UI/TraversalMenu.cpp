//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalMenu.h"
#include "TraversalMenuListener.h"
#include "../Util/ApplicationContext.h"
#include "../Graph/ValueTreeState.h"
#include "Theme/CustomLookAndFeel.h"

TraversalMenu::TraversalMenu(ApplicationContext& context, bool showResizer)
    : ResizablePanel(context, ResizeEdge::Left, resizerWidth, showResizer),
      displayMenu(context), multiplierEditor(context), channelEditor(context), transposeEditor(context), velocityEditor(context), colourSelector(context) {
    addAndMakeVisible(displayMenu);

    multiplierLabel.setText("Multiplier", juce::dontSendNotification);
    multiplierLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    multiplierLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    multiplierLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(multiplierLabel);

    multiplierEditor.enableDecimalValue(0.1);
    addAndMakeVisible(multiplierEditor);

    channelLabel.setText("Channel", juce::dontSendNotification);
    channelLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    channelLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    channelLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(channelLabel);

    channelEditor.setMinimumValue(1);
    addAndMakeVisible(channelEditor);

    transposeLabel.setText("Transpose", juce::dontSendNotification);
    transposeLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    transposeLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    transposeLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(transposeLabel);

    transposeEditor.enableSignedValue(-24, 24);
    addAndMakeVisible(transposeEditor);

    velocityLabel.setText("Velocity", juce::dontSendNotification);
    velocityLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    velocityLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    velocityLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(velocityLabel);

    velocityEditor.enableDecimalValue(0.0, 1.0);
    addAndMakeVisible(velocityEditor);

    colourLabel.setText("Colour", juce::dontSendNotification);
    colourLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    colourLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    colourLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(colourLabel);

    colourSelector.requiresNode = false;
    colourSelector.onColourPicked = [this](juce::Colour c) {
        if (currentTraversalData.isValid()) {
            currentTraversalData.setProperty(ValueTreeIdentifiers::TraversalColour, c.toString(), nullptr);
        }
    };
    addAndMakeVisible(colourSelector);


    editTraversalRulesButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTextButton(g, bounds, state);
        }, context.lookAndFeel);

    editTraversalRulesButton->setText("edit traversal rules");
    editTraversalRulesButton->onClick = [this]() { traversalRulesLauncher.show(); };

    addAndMakeVisible(editTraversalRulesButton.get());

    displayMenu.onTraversalSelected = [this](int traversalId) {
        selectTraversal(traversalId);
    };

    menuListener = std::make_unique<TraversalMenuListener>(*this);
    applicationContext.valueTreeState->traversalMap.addListener(menuListener.get());

    int firstTraversalId = -1;

    for (int i = 0; i < applicationContext.valueTreeState->traversalMap.getNumChildren(); ++i) {
        juce::ValueTree traversalData = applicationContext.valueTreeState->traversalMap.getChild(i);
        if (traversalData.getType() == ValueTreeIdentifiers::TraversalData) {
            int traversalId = traversalData.getProperty(ValueTreeIdentifiers::TraversalId);
            displayMenu.addTraversalToMenu(traversalId);
            if (firstTraversalId == -1) {
                firstTraversalId = traversalId;
            }
        }
    }

    if (firstTraversalId != -1) {
        selectTraversal(firstTraversalId);
    }
}

void TraversalMenu::selectTraversal(int traversalId) {
    juce::ValueTree traversalData = applicationContext.valueTreeState->traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

    if (!traversalData.isValid()) {
        return;
    }

    currentTraversalData = traversalData;

    multiplierEditor.bindEditor(traversalData, ValueTreeIdentifiers::TempoMultiplier);

    if (!traversalData.hasProperty(ValueTreeIdentifiers::TraversalChannel)) {
        traversalData.setProperty(ValueTreeIdentifiers::TraversalChannel, 1, nullptr);
    }
    channelEditor.bindEditor(traversalData, ValueTreeIdentifiers::TraversalChannel);

    if (!traversalData.hasProperty(ValueTreeIdentifiers::TraversalTranspose)) {
        traversalData.setProperty(ValueTreeIdentifiers::TraversalTranspose, 0, nullptr);
    }
    transposeEditor.bindEditor(traversalData, ValueTreeIdentifiers::TraversalTranspose);

    if (!traversalData.hasProperty(ValueTreeIdentifiers::TraversalVelocity)) {
        traversalData.setProperty(ValueTreeIdentifiers::TraversalVelocity, 1.0, nullptr);
    }
    velocityEditor.bindEditor(traversalData, ValueTreeIdentifiers::TraversalVelocity);

    juce::String colourString = traversalData.getProperty(ValueTreeIdentifiers::TraversalColour).toString();
    colourSelector.colour = colourString.isNotEmpty() ? juce::Colour::fromString(colourString) : juce::Colours::white;
    colourSelector.repaint();

    displayMenu.selectedOption = "Traversal " + juce::String(traversalId);
    displayMenu.resized();
    displayMenu.repaint();
}

TraversalMenu::~TraversalMenu() {
    applicationContext.valueTreeState->traversalMap.removeListener(menuListener.get());
}

void TraversalMenu::paint(juce::Graphics &g) {
    ResizablePanel::paint(g);

    auto bounds = getLocalBounds().toFloat();
    auto barHeight = std::floor(bounds.getHeight() * 0.05f);
    auto barBounds = bounds.withHeight(barHeight);

    if (hasResizer()) {
        barBounds = barBounds.withTrimmedLeft((float) resizerWidth);
    }

    drawTopBar(g, barBounds);
}

void TraversalMenu::resized() {
    auto bounds = getLocalBounds();

    if (hasResizer()) {
        resizer.setBounds(bounds.removeFromLeft(resizerWidth));
    }

    auto editRulesArea = bounds.removeFromBottom(Theme::textButtonHeight + Theme::menuEdgeInset * 2);
    editTraversalRulesButton->setBounds(editRulesArea.reduced(Theme::menuEdgeInset));

    int barHeight = static_cast<int>(getHeight() * 0.05f);
    auto barArea = bounds.removeFromTop(barHeight);
    displayMenu.setBounds(barArea.reduced(4));

    int rowHeight = juce::jmax(18, barHeight);
    auto rowArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
    multiplierLabel.setBounds(rowArea.removeFromLeft(rowArea.getWidth() / 2));
    multiplierEditor.setBounds(rowArea);

    auto channelArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
    channelLabel.setBounds(channelArea.removeFromLeft(channelArea.getWidth() / 2));
    channelEditor.setBounds(channelArea);

    auto transposeArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
    transposeLabel.setBounds(transposeArea.removeFromLeft(transposeArea.getWidth() / 2));
    transposeEditor.setBounds(transposeArea);

    auto velocityArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
    velocityLabel.setBounds(velocityArea.removeFromLeft(velocityArea.getWidth() / 2));
    velocityEditor.setBounds(velocityArea);

    auto colourArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
    colourLabel.setBounds(colourArea.removeFromLeft(colourArea.getWidth() / 2));
    colourSelector.setBounds(colourArea);
}
