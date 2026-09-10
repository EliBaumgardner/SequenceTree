//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalMenu.h"
#include "TraversalMenuListener.h"
#include "../../Util/ApplicationContext.h"
#include "../../Graph/GraphState.h"
#include "../Theme/CustomLookAndFeel.h"

TraversalMenu::TraversalMenu(ApplicationContext& context)
    : displayMenu(context), multiplierEditor(context), channelEditor(context), transposeEditor(context), velocityEditor(context), colourSelector(context),
      applicationContext(context),
      topBar(context, { Bar::Orientation::horizontal, Bar::Background::litFromTop }) {
    setLookAndFeel(context.lookAndFeel);

    addAndMakeVisible(topBar);
    addAndMakeVisible(displayMenu);

    const auto setUpLabel = [this](juce::Label& label, juce::String text) {
        label.setText(std::move(text), juce::dontSendNotification);
        label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
        label.setFont(juce::Font(juce::FontOptions(9.0f)));
        label.setJustificationType(juce::Justification::centredLeft);
        addAndMakeVisible(label);
    };

    setUpLabel(multiplierLabel, "Multiplier");
    multiplierEditor.enableDecimalValue(0.1);
    addAndMakeVisible(multiplierEditor);

    setUpLabel(channelLabel, "Channel");
    channelEditor.setMinimumValue(1);
    addAndMakeVisible(channelEditor);

    setUpLabel(transposeLabel, "Transpose");
    transposeEditor.enableSignedValue(-24, 24);
    addAndMakeVisible(transposeEditor);

    setUpLabel(velocityLabel, "Velocity");
    velocityEditor.enableDecimalValue(0.0, 1.0);
    addAndMakeVisible(velocityEditor);

    setUpLabel(colourLabel, "Colour");

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

    displayMenu.onItemSelected = [this](int traversalId) {
        selectTraversal(traversalId);
    };

    menuListener = std::make_unique<TraversalMenuListener>(*this);
    applicationContext.graphState->traversals.map.addListener(menuListener.get());

    int firstTraversalId = -1;

    for (int i = 0; i < applicationContext.graphState->traversals.map.getNumChildren(); ++i) {
        const juce::ValueTree traversalData = applicationContext.graphState->traversals.map.getChild(i);
        if (traversalData.getType() == ValueTreeIdentifiers::TraversalData) {
            const int traversalId = traversalData.getProperty(ValueTreeIdentifiers::TraversalId);
            addTraversalToMenu(traversalId);
            if (firstTraversalId == -1) {
                firstTraversalId = traversalId;
            }
        }
    }

    if (firstTraversalId != -1) {
        selectTraversal(firstTraversalId);
    }
}

void TraversalMenu::addTraversalToMenu(int traversalId) {
    displayMenu.addItem(traversalId, "Traversal " + juce::String(traversalId));
}

void TraversalMenu::selectTraversal(int traversalId) {
    juce::ValueTree traversalData = applicationContext.graphState->traversals.map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

    if (!traversalData.isValid()) {
        return;
    }

    currentTraversalData = traversalData;

    const auto bindWithDefault = [&traversalData](ValueEditor& editor, const juce::Identifier& propertyId,
                                           const juce::var& defaultValue) {
        if (!traversalData.hasProperty(propertyId)) {
            traversalData.setProperty(propertyId, defaultValue, nullptr);
        }
        editor.bindEditor(traversalData, propertyId);
    };

    multiplierEditor.bindEditor(traversalData, ValueTreeIdentifiers::TempoMultiplier);

    bindWithDefault(channelEditor,   ValueTreeIdentifiers::TraversalChannel,   1);
    bindWithDefault(transposeEditor, ValueTreeIdentifiers::TraversalTranspose, 0);
    bindWithDefault(velocityEditor,  ValueTreeIdentifiers::TraversalVelocity,  1.0);

    const juce::String colourString = traversalData.getProperty(ValueTreeIdentifiers::TraversalColour).toString();
    if (colourString.isNotEmpty()) {
        colourSelector.colour = juce::Colour::fromString(colourString);
    } else {
        colourSelector.colour = juce::Colours::white;
    }

    colourSelector.repaint();

    displayMenu.setSelectedItem(traversalId);
}

TraversalMenu::~TraversalMenu() {
    applicationContext.graphState->traversals.map.removeListener(menuListener.get());
    setLookAndFeel(nullptr);
}

void TraversalMenu::paint(juce::Graphics &g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());
}

void TraversalMenu::resized() {
    auto bounds = getLocalBounds();

    auto editRulesArea = bounds.removeFromBottom(Theme::textButtonHeight + Theme::menuEdgeInset * 2);
    editTraversalRulesButton->setBounds(editRulesArea.reduced(Theme::menuEdgeInset));

    int barHeight = static_cast<int>(getHeight() * 0.05f);
    auto barArea = bounds.removeFromTop(barHeight);
    topBar.setBounds(barArea);
    displayMenu.setBounds(barArea.reduced(4));

    int rowHeight = juce::jmax(18, barHeight);

    auto layoutRow = [&bounds, rowHeight](juce::Label& label, juce::Component& control) {
        auto rowArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
        label.setBounds(rowArea.removeFromLeft(rowArea.getWidth() / 2));
        control.setBounds(rowArea);
    };

    layoutRow(multiplierLabel, multiplierEditor);
    layoutRow(channelLabel,    channelEditor);
    layoutRow(transposeLabel,  transposeEditor);
    layoutRow(velocityLabel,   velocityEditor);
    layoutRow(colourLabel,     colourSelector);
}
