#include "MenuArea.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"
#include "../Bars/MenuBar.h"
#include "TraversalMenu.h"
#include "NodeMenu.h"

MenuArea::MenuArea(NodeCanvas& nodeCanvas, TraversalMenu& traversalMenu, NodeMenu& nodeMenu)
    : resizer      (ArsNovaUI::PanelResizer::Edge::Right),
      topBar       (nodeCanvas, ArsNovaUI::Bar::Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      nodeCanvas   (nodeCanvas),
      traversalMenu(traversalMenu),
      nodeMenu     (nodeMenu)
{
    menuBar = std::make_unique<MenuBar>(nodeCanvas);

    menuBar->traversalIcon.onClick = [this] { togglePanel(ActivePanel::Traversal); };
    menuBar->nodeIcon.onClick      = [this] { togglePanel(ActivePanel::Node); };

    addAndMakeVisible(topBar);
    addAndMakeVisible(menuBar.get());

    addChildComponent(traversalMenu);
    addChildComponent(nodeMenu);

    addAndMakeVisible(resizer);
}

MenuArea::~MenuArea() = default;

void MenuArea::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    lookAndFeel.drawFrostedGlass(graphics, *this, nodeCanvas, resizer.getBounds());
    lookAndFeel.drawFrostedGlass(graphics, *this, nodeCanvas, getLocalBounds().withRight(menuBar->getX()).withTrimmedTop(topBar.getBottom()));
}

void MenuArea::resized()
{
    const int barHeight = ArsNovaUI::Theme::barHeight;

    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(juce::roundToInt(barHeight * resizerWidthRatio)));

    auto menuColumn = bounds.removeFromRight(juce::roundToInt(barHeight * menuBarWidthRatio));

    menuBar->setBounds(menuColumn);
    topBar.setBounds(bounds.withHeight(barHeight));
    traversalMenu.setBounds(bounds);
    nodeMenu.setBounds(bounds);
}

int MenuArea::narrowestHeight() const
{
    const int barHeight    = ArsNovaUI::Theme::barHeight;
    const int menuBarWidth = juce::roundToInt(barHeight * menuBarWidthRatio);
    const int iconInset    = juce::roundToInt(menuBarWidth * MenuBar::iconInsetRatio);
    const int iconSize     = menuBarWidth - iconInset * 2;
    const int iconGap      = juce::roundToInt(iconSize * ArsNovaUI::Theme::iconGapRatio);
    const int columnHeight = iconInset * 2 + iconSize * 3 + iconGap * 2;

    return juce::jmax(columnHeight, traversalMenu.narrowestHeight(), nodeMenu.narrowestHeight());
}

void MenuArea::togglePanel(ActivePanel panel)
{
    if (activePanel == panel) {
        activePanel = ActivePanel::None;
    }
    else {
        activePanel = panel;
    }

    traversalMenu.setVisible(activePanel == ActivePanel::Traversal);
    nodeMenu.setVisible(activePanel == ActivePanel::Node);

    menuBar->traversalIcon.setSelected(activePanel == ActivePanel::Traversal);
    menuBar->nodeIcon.setSelected(activePanel == ActivePanel::Node);

    resized();

    menuBar->repaint();
}
