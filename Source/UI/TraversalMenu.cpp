//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalMenu.h"
#include "TraversalMenuListener.h"
#include "../Util/ApplicationContext.h"
#include "../Graph/ValueTreeState.h"
#include "Theme/CustomLookAndFeel.h"

TraversalMenu::TraversalMenu(ApplicationContext& context) : displayMenu(context), multiplierEditor(context) {
    setLookAndFeel(context.lookAndFeel);
    addAndMakeVisible(displayMenu);
    addAndMakeVisible(resizer);

    multiplierLabel.setText("Multiplier", juce::dontSendNotification);
    multiplierLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    multiplierLabel.setFont(juce::Font(juce::FontOptions(9.0f)));
    multiplierLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(multiplierLabel);

    multiplierEditor.enableDecimalValue(0.1);
    addAndMakeVisible(multiplierEditor);

    displayMenu.onTraversalSelected = [this](int traversalId) {
        selectTraversal(traversalId);
    };

    menuListener = std::make_unique<TraversalMenuListener>(*this);
    ValueTreeState::traversalMap.addListener(menuListener.get());

    int firstTraversalId = -1;

    for (int i = 0; i < ValueTreeState::traversalMap.getNumChildren(); ++i) {
        juce::ValueTree traversalData = ValueTreeState::traversalMap.getChild(i);
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
    juce::ValueTree traversalData = ValueTreeState::traversalMap.getChildWithProperty(ValueTreeIdentifiers::TraversalId, traversalId);

    if (!traversalData.isValid()) {
        return;
    }

    multiplierEditor.bindEditor(traversalData, ValueTreeIdentifiers::TempoMultiplier);

    displayMenu.selectedOption = "Traversal " + juce::String(traversalId);
    displayMenu.resized();
    displayMenu.repaint();
}

TraversalMenu::~TraversalMenu() {
    ValueTreeState::traversalMap.removeListener(menuListener.get());
}

void TraversalMenu::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawTraversalMenu(g, *this);
}

void TraversalMenu::resized() {
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromLeft(resizerWidth));

    int barHeight = static_cast<int>(getHeight() * 0.05f);
    auto barArea = bounds.removeFromTop(barHeight);
    displayMenu.setBounds(barArea.reduced(4));

    int rowHeight = juce::jmax(18, barHeight);
    auto rowArea = bounds.removeFromTop(rowHeight).reduced(4, 2);
    multiplierLabel.setBounds(rowArea.removeFromLeft(rowArea.getWidth() / 2));
    multiplierEditor.setBounds(rowArea);
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
    CustomLookAndFeel::get(*this).drawTraversalMenuResizer(g, getLocalBounds(), isHovered, isDragging);
}
