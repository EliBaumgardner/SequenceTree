//
// Created by Eli Baumgardner on 7/17/26.
//

#include "NodeMenu.h"
#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"

NodeMenu::NodeMenu(ApplicationContext& context) {
    setLookAndFeel(context.lookAndFeel);
    addAndMakeVisible(resizer);
}

NodeMenu::~NodeMenu() {
    setLookAndFeel(nullptr);
}

void NodeMenu::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawNodeMenu(g, *this);
}

void NodeMenu::resized() {
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(resizerWidth));
}

NodeMenu::Resizer::Resizer(NodeMenu& ownerRef) : owner(ownerRef)
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void NodeMenu::Resizer::mouseDown(const juce::MouseEvent& e)
{
    dragStartWidth = owner.getWidth();
    dragStartX     = owner.getX();
    isDragging     = true;
    repaint();
}

void NodeMenu::Resizer::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    repaint();
}

void NodeMenu::Resizer::mouseEnter(const juce::MouseEvent& e)
{
    isHovered = true;
    repaint();
}

void NodeMenu::Resizer::mouseExit(const juce::MouseEvent& e)
{
    isHovered = false;
    repaint();
}

void NodeMenu::Resizer::mouseDrag(const juce::MouseEvent& e)
{
    int deltaX = e.getScreenPosition().getX() - e.getMouseDownScreenPosition().getX();

    int newWidth = juce::jmax(minMenuWidth, dragStartWidth + deltaX);

    if (owner.onWidthDragged) {
        owner.onWidthDragged(newWidth);
    }
    else {
        owner.setBounds(dragStartX, owner.getY(), newWidth, owner.getHeight());
    }
}

void NodeMenu::Resizer::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawNodeMenuResizer(g, getLocalBounds(), isHovered, isDragging);
}
