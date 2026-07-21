//
// Created by Eli Baumgardner on 7/17/26.
//

#include "MenuArea.h"
#include "../Util/ApplicationContext.h"
#include "Theme/CustomLookAndFeel.h"
#include "../UI/MenuBar.h"
#include "../UI/TraversalMenu.h"
#include "../UI/NodeMenu.h"

MenuArea::MenuArea(ApplicationContext& context) {
    setLookAndFeel(context.lookAndFeel);

    menuBar = std::make_unique<MenuBar>(context);
    menuBar->traversalIcon->onClick = [this] { togglePanel(ActivePanel::Traversal); };
    menuBar->nodeIcon->onClick      = [this] { togglePanel(ActivePanel::Node); };

    traversalMenu = std::make_unique<TraversalMenu>(context, false);
    nodeMenu      = std::make_unique<NodeMenu>(context);

    addAndMakeVisible(resizer);
    addAndMakeVisible(menuBar.get());
    addChildComponent(traversalMenu.get());
    addChildComponent(nodeMenu.get());
}

MenuArea::~MenuArea() {
    setLookAndFeel(nullptr);
}

void MenuArea::paint(juce::Graphics &g) {
    CustomLookAndFeel::get(*this).drawMenuArea(g, *this);
}

void MenuArea::resized() {
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(resizerWidth));
    menuBar->setBounds(bounds.removeFromRight(menuBarWidth));

    traversalMenu->setBounds(bounds);
    nodeMenu->setBounds(bounds);
}

void MenuArea::togglePanel(ActivePanel panel) {
    activePanel = (activePanel == panel) ? ActivePanel::None : panel;

    traversalMenu->setVisible(activePanel == ActivePanel::Traversal);
    nodeMenu->setVisible(activePanel == ActivePanel::Node);

    resized();
}

MenuArea::Resizer::Resizer(MenuArea& ownerRef) : owner(ownerRef)
{
    setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
}

void MenuArea::Resizer::mouseDown(const juce::MouseEvent& e)
{
    dragStartWidth = owner.getWidth();
    dragStartX     = owner.getX();
    isDragging     = true;
    repaint();
}

void MenuArea::Resizer::mouseUp(const juce::MouseEvent& e)
{
    isDragging = false;
    repaint();
}

void MenuArea::Resizer::mouseEnter(const juce::MouseEvent& e)
{
    isHovered = true;
    repaint();
}

void MenuArea::Resizer::mouseExit(const juce::MouseEvent& e)
{
    isHovered = false;
    repaint();
}

void MenuArea::Resizer::mouseDrag(const juce::MouseEvent& e)
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

void MenuArea::Resizer::paint(juce::Graphics& g)
{
    CustomLookAndFeel::get(*this).drawMenuAreaResizer(g, getLocalBounds(), isHovered, isDragging);
}
