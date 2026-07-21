//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalMenu.h"
#include "TraversalMenuListener.h"
#include "../Util/ApplicationContext.h"
#include "../Graph/ValueTreeState.h"
#include "Theme/CustomLookAndFeel.h"

TraversalMenu::TraversalMenu(ApplicationContext& context, bool showResizer) : displayMenu(context), multiplierEditor(context), channelEditor(context), transposeEditor(context), velocityEditor(context), colourSelector(context), editTraversalRulesButton(context), applicationContext(context), showResizer(showResizer) {
    setLookAndFeel(context.lookAndFeel);
    addAndMakeVisible(displayMenu);
    addChildComponent(resizer);
    resizer.setVisible(showResizer);

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

    addAndMakeVisible(editTraversalRulesButton);

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
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat();

    g.setColour(theme.baseDarkColour2);
    g.fillRect(bounds);

    auto barHeight = std::floor(bounds.getHeight() * 0.05f);
    auto barBounds = bounds.withHeight(barHeight);

    if (showResizer) {
        barBounds = barBounds.withTrimmedLeft((float) resizerWidth);
    }

    juce::ColourGradient gradient(theme.barColour.brighter(0.06f), 0, barBounds.getY(),
                                  theme.barColour.darker(0.04f),   0, barBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(barBounds);

    g.setColour(theme.barColour.brighter(0.12f));
    g.drawHorizontalLine((int) barBounds.getY(), barBounds.getX(), barBounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int) barBounds.getBottom() - 1, barBounds.getX(), barBounds.getRight());
}

void TraversalMenu::resized() {
    auto bounds = getLocalBounds();

    if (showResizer) {
        resizer.setBounds(bounds.removeFromLeft(resizerWidth));
    }

    auto editRulesArea = bounds.removeFromBottom(MenuTextButton::preferredHeight + MenuTextButton::menuEdgeInset * 2);
    editTraversalRulesButton.setBounds(editRulesArea.reduced(MenuTextButton::menuEdgeInset));

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

TraversalMenu::Resizer::Resizer(TraversalMenu& ownerRef) : owner(ownerRef)
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void TraversalMenu::Resizer::mouseDown(const juce::MouseEvent& e)
{
    dragStartWidth = owner.getWidth();
    dragStartX     = owner.getX();
    isDragging     = true;
    repaint();
}

void TraversalMenu::Resizer::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    repaint();
}

void TraversalMenu::Resizer::mouseEnter(const juce::MouseEvent& e)
{
    isHovered = true;
    repaint();
}

void TraversalMenu::Resizer::mouseExit(const juce::MouseEvent& e)
{
    isHovered = false;
    repaint();
}

void TraversalMenu::Resizer::mouseDrag(const juce::MouseEvent& e)
{
    int deltaX = e.getScreenPosition().getX() - e.getMouseDownScreenPosition().getX();

    int newWidth = juce::jmax(minMenuWidth, dragStartWidth - deltaX);

    if (owner.onWidthDragged) {
        owner.onWidthDragged(newWidth);
    }
    else {
        int newX = dragStartX + (dragStartWidth - newWidth);
        owner.setBounds(newX, owner.getY(), newWidth, owner.getHeight());
    }
}

void TraversalMenu::Resizer::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds();

    juce::Colour fill = theme.baseDarkColour1;

    if (isDragging) {
        fill = theme.baseLightColour2;
    }
    else if (isHovered) {
        fill = theme.baseDarkColour1.brighter(0.25f);
    }

    g.setColour(fill);
    g.fillRect(bounds);

    auto gripBounds = bounds.toFloat().reduced(bounds.getWidth() * 0.3f, bounds.getHeight() * 0.35f);
    g.setColour(theme.baseLightColour1.withAlpha(0.5f));

    const float dotSpacing = 4.0f;
    for (float y = gripBounds.getY(); y < gripBounds.getBottom(); y += dotSpacing) {
        g.fillRect(gripBounds.getX(), y, gripBounds.getWidth(), 1.0f);
    }
}
