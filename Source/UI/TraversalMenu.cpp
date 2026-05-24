//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalMenu.h"
#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

TraversalMenu::TraversalMenu(ApplicationContext& context) : displayMenu(context) {
    setLookAndFeel(context.lookAndFeel);
    addAndMakeVisible(displayMenu);
    addAndMakeVisible(resizer);
}

void TraversalMenu::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawTraversalMenu(g, *this);
}

void TraversalMenu::resized() {
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromLeft(resizerWidth));

    auto menuArea = bounds.removeFromTop(displayMenuSize);
    displayMenu.setBounds(menuArea);
}

TraversalMenu::Resizer::Resizer(TraversalMenu& ownerRef) : owner(ownerRef)
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void TraversalMenu::Resizer::mouseDown(const juce::MouseEvent& e)
{
    dragStartWidth = owner.getWidth();
    dragStartX     = owner.getX();
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
    g.setColour(juce::Colours::black.withAlpha(0.25f));
    g.fillRect(getLocalBounds());
}
