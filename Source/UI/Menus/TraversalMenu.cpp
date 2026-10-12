#include "TraversalMenu.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/NodeInfo.h"
#include "../Canvas/NodeCanvas.h"

TraversalMenu::TraversalMenu(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                             juce::ValueTree colourPresets, juce::UndoManager& undoManager)
    : displayMenu(undoManager),
      multiplierField(undoManager),
      channelField(undoManager),
      transposeField(undoManager),
      velocityField(undoManager),
      colourSelector(colourPresets, undoManager),
      nodeCanvas(nodeCanvas),
      graphState(graphState),
      traversalRuleState(traversalRuleState),
      snapshots(snapshots),
      undoManager(undoManager)
{
    const juce::ValueTree traversalMap     = graphState.traversals.map;
    auto                  transposeFormat  = std::make_unique<NumberFormat>(minimumTraversalTranspose, maximumTraversalTranspose);
    int                   firstTraversalId = -1;

    editTraversalRulesButton.painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state) {
        CustomLookAndFeel::get(*this).drawCodeButton(graphics, bounds, state, Theme::textHeight);
    };

    displayMenu.labelEditor.autoFitText = false;
    transposeFormat->showsPositiveSign  = true;

    multiplierField.label.setText("Multiplier", juce::dontSendNotification);
    channelField.label.setText("Channel", juce::dontSendNotification);
    transposeField.label.setText("Transpose", juce::dontSendNotification);
    velocityField.label.setText("Velocity", juce::dontSendNotification);
    colourLabel.setText("Colour", juce::dontSendNotification);

    multiplierField.editor.setFormat(std::make_unique<NumberFormat>(minimumTraversalMultiplier, RTtraversal::maximumTempoMultiplier,
                                                                    ValueFormat::editableDecimalPlaces));
    channelField.editor.setFormat(std::make_unique<NumberFormat>(minimumMidiChannel, maximumMidiChannel));
    transposeField.editor.setFormat(std::move(transposeFormat));
    velocityField.editor.setFormat(std::make_unique<NumberFormat>(0.0, 1.0, ValueFormat::editableDecimalPlaces));

    editTraversalRulesButton.setText("edit traversal rules");

    colourSelector.onColourPicked = [this](juce::Colour pickedColour) {
        if (currentTraversalData.isValid()) {
            currentTraversalData.setProperty(ValueTreeIdentifiers::TraversalColour, pickedColour.toString(), nullptr);
        }
    };

    displayMenu.onItemSelected = [this](int traversalId) { selectTraversal(traversalId); };

    editTraversalRulesButton.onClick = [this]() { traversalRulesLauncher.show(*this); };

    addChildComponent(displayMenu);
    addChildComponent(multiplierField);
    addChildComponent(channelField);
    addChildComponent(transposeField);
    addChildComponent(velocityField);
    addChildComponent(colourLabel);
    addChildComponent(colourSelector);
    addChildComponent(editTraversalRulesButton);

    graphState.traversals.map.addListener(this);

    for (int traversalIndex = 0; traversalIndex < traversalMap.getNumChildren(); ++traversalIndex) {
        const juce::ValueTree traversalData = traversalMap.getChild(traversalIndex);

        if (traversalData.getType() != ValueTreeIdentifiers::TraversalData) {
            continue;
        }

        const int traversalId = traversalData.getProperty(ValueTreeIdentifiers::TraversalId);

        displayMenu.addItem(traversalId, "Traversal " + juce::String(traversalId));

        if (firstTraversalId == -1) {
            firstTraversalId = traversalId;
        }
    }

    if (firstTraversalId != -1) {
        selectTraversal(firstTraversalId);
    }
}

TraversalMenu::~TraversalMenu()
{
    graphState.traversals.map.removeListener(this);
}

void TraversalMenu::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);
    const int          barHeight   = Theme::barHeight;
    const int          spacing     = juce::roundToInt(barHeight * Theme::menuSpacingRatio);

    if (! currentTraversalData.isValid()) {
        graphics.setColour(lookAndFeel.mutedTextColour);
        graphics.setFont(lookAndFeel.font(Theme::FontStyle::Regular, Theme::textHeight));
        graphics.drawFittedText("Shift + click the canvas to create a node", getLocalBounds().reduced(spacing), juce::Justification::centred, 2);

        return;
    }

    lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());

    graphics.setColour(lookAndFeel.borderColour);
    graphics.fillRect(0.0f, static_cast<float>(barHeight), static_cast<float>(getWidth()), Theme::borderThickness);
}

void TraversalMenu::resized()
{
    const int        barHeight  = Theme::barHeight;
    const int        spacing    = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    const int        rowHeight  = juce::roundToInt(barHeight * Theme::menuRowHeightRatio);
    const float      textHeight = Theme::textHeight;
    const juce::Font textFont   = CustomLookAndFeel::get(*this).font(Theme::FontStyle::Regular, textHeight);
    auto             bounds     = getLocalBounds();
    auto             barArea    = bounds.removeFromTop(barHeight);

    displayMenu.labelEditor.setFontHeight(textHeight);
    multiplierField.editor.setFontHeight(textHeight);
    channelField.editor.setFontHeight(textHeight);
    transposeField.editor.setFontHeight(textHeight);
    velocityField.editor.setFontHeight(textHeight);

    displayMenu.setBounds(barArea.reduced(juce::roundToInt(barHeight * Theme::contentInsetRatio)).withWidth(Theme::menuContentWidth));

    bounds = bounds.reduced(spacing).withWidth(Theme::menuContentWidth);

    editTraversalRulesButton.setBounds(bounds.removeFromBottom(juce::roundToInt(barHeight * Theme::menuButtonHeightRatio)));

    for (LabeledEditor* field : { &multiplierField, &channelField, &transposeField, &velocityField }) {
        auto rowArea = bounds.removeFromTop(rowHeight);

        field->label.setFont(textFont);

        field->labelWidth = rowArea.getWidth() / 2;

        field->setBounds(rowArea);

        bounds.removeFromTop(spacing);
    }

    auto colourRow = bounds.removeFromTop(rowHeight);

    colourLabel.setFont(textFont);

    colourLabel.setBounds(colourRow.removeFromLeft(colourRow.getWidth() / 2));
    colourSelector.setBounds(colourRow);
}

void TraversalMenu::selectTraversal(int traversalId)
{
    juce::ValueTree    traversalData = graphState.traversals.map.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);
    const juce::String colourString  = traversalData.getProperty(ValueTreeIdentifiers::TraversalColour).toString();

    const auto bindWithDefault = [&traversalData](ValueEditor& editor, const juce::Identifier& propertyId,
                                                  const juce::var& defaultValue) {
        if (!traversalData.hasProperty(propertyId)) {
            traversalData.setProperty(propertyId, defaultValue, nullptr);
        }

        editor.bindEditor(traversalData, propertyId);
    };

    if (!traversalData.isValid()) {
        return;
    }

    currentTraversalData     = traversalData;
    nodeCanvas.quaverTraversalId = traversalId;

    for (juce::Component* content : getChildren()) {
        content->setVisible(true);
    }

    repaint();

    multiplierField.editor.bindEditor(traversalData, ValueTreeIdentifiers::TempoMultiplier);

    bindWithDefault(channelField.editor,   ValueTreeIdentifiers::TraversalChannel,   TraversalState::defaultChannel);
    bindWithDefault(transposeField.editor, ValueTreeIdentifiers::TraversalTranspose, TraversalState::defaultTranspose);
    bindWithDefault(velocityField.editor,  ValueTreeIdentifiers::TraversalVelocity,  TraversalState::defaultVelocity);

    if (colourString.isNotEmpty()) {
        colourSelector.colour = juce::Colour::fromString(colourString);
    }
    else {
        colourSelector.colour = juce::Colours::white;
    }

    colourSelector.repaint();

    displayMenu.setSelectedItem(traversalId);
}

int TraversalMenu::narrowestHeight() const
{
    const int barHeight     = Theme::barHeight;
    const int spacing       = juce::roundToInt(barHeight * Theme::menuSpacingRatio);
    const int rowHeight     = juce::roundToInt(barHeight * Theme::menuRowHeightRatio);
    const int buttonHeight  = juce::roundToInt(barHeight * Theme::menuButtonHeightRatio);
    const int fieldRowCount = 4;

    return barHeight + spacing * 2 + buttonHeight + fieldRowCount * (rowHeight + spacing) + rowHeight;
}

void TraversalMenu::valueTreeChildAdded(juce::ValueTree&, juce::ValueTree& child)
{
    if (child.getType() != ValueTreeIdentifiers::TraversalData) {
        return;
    }

    const int traversalId = child.getProperty(ValueTreeIdentifiers::TraversalId);

    displayMenu.addItem(traversalId, "Traversal " + juce::String(traversalId));

    if (displayMenu.selectedLabel.isEmpty()) {
        selectTraversal(traversalId);
    }
}

void TraversalMenu::valueTreeChildRemoved(juce::ValueTree&, juce::ValueTree& child, int)
{
    const juce::ValueTree remainingTraversal = graphState.traversals.map.getChildWithName(ValueTreeIdentifiers::TraversalData);

    if (child.getType() != ValueTreeIdentifiers::TraversalData) {
        return;
    }

    displayMenu.removeItem(child.getProperty(ValueTreeIdentifiers::TraversalId));

    if (child != currentTraversalData) {
        return;
    }

    if (remainingTraversal.isValid()) {
        selectTraversal(remainingTraversal.getProperty(ValueTreeIdentifiers::TraversalId));

        return;
    }

    currentTraversalData = juce::ValueTree();

    for (juce::Component* content : getChildren()) {
        content->setVisible(false);
    }

    repaint();
}
