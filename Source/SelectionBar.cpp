/*
  ==============================================================================

    SelectionBar.cpp
    Created: 27 Aug 2025 6:30:12pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "SelectionBar.h"

SelectionBar::SelectionBar(){
    
    addAndMakeVisible(selectionButton);
    addAndMakeVisible(pianoRollButton);
    addAndMakeVisible(nodeButton);
    addAndMakeVisible(inspectButton);
    
    nodeButton.onClick = [this]() {
        nodeButton.buttonSelected = true;
        inspectButton.buttonSelected = false;
        
        nodeButton.repaint();
        inspectButton.repaint();
        
        ComponentContext::canvas->controllerMode = NodeCanvas::ControllerMode::Node;
    };
    
    inspectButton.onClick = [this]() {
        inspectButton.buttonSelected = true;
        nodeButton.buttonSelected = false;
        
        nodeButton.repaint();
        inspectButton.repaint();
        
        ComponentContext::canvas->controllerMode = NodeCanvas::ControllerMode::Inspect;
    };
}

void SelectionBar::resized() {
    // The drawable area inside the border
    auto innerBounds = getLocalBounds().reduced(2);

    std::array<juce::Component*, 3> buttons = {
        &inspectButton,
        &nodeButton,
        &pianoRollButton
    };
    
    // Decide how much space on the right for the button strip
    int buttonStripWidth = innerBounds.getWidth() / 3;   // 1/4 of width reserved for buttons
    int buttonWidth      = buttonStripWidth / buttons.size();
    int buttonHeight     = innerBounds.getHeight();

    // Extract the right side of the innerBounds for the buttons
    auto buttonArea = innerBounds.removeFromRight(buttonStripWidth);

    for (auto* b : buttons)
        b->setBounds(buttonArea.removeFromLeft(buttonWidth));
    
    selectionButton.setBounds(innerBounds.removeFromLeft(buttonWidth*2));
    
}

void SelectionBar::paint(juce::Graphics& g) {
    
    auto bounds = getLocalBounds().reduced(2.0f).toFloat();
    g.setColour(juce::Colours::black);
    g.drawRect(bounds,2.0f);
}
