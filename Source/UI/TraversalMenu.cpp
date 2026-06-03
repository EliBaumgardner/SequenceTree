//
// Created by Eli Baumgardner on 5/23/26.
//

#include "TraversalMenu.h"
#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

TraversalMenu::TraversalMenu(ApplicationContext& context) :displayMenu(context) {
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

    int barHeight = static_cast<int>(getHeight() * 0.05f);
    auto barArea = bounds.removeFromTop(barHeight);
    displayMenu.setBounds(barArea.reduced(4));
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
