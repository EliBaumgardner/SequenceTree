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
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat();

    g.setColour(theme.baseDarkColour2);
    g.fillRect(bounds);

    auto barHeight = std::floor(bounds.getHeight() * 0.05f);
    auto barBounds = bounds.withHeight(barHeight).withTrimmedRight((float) resizerWidth);

    juce::ColourGradient gradient(theme.barColour.brighter(0.06f), 0, barBounds.getY(),
                                  theme.barColour.darker(0.04f),   0, barBounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(barBounds);

    g.setColour(theme.barColour.brighter(0.12f));
    g.drawHorizontalLine((int) barBounds.getY(), barBounds.getX(), barBounds.getRight());

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int) barBounds.getBottom() - 1, barBounds.getX(), barBounds.getRight());
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
