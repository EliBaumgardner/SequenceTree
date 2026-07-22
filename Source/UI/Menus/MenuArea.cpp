//
// Created by Eli Baumgardner on 7/17/26.
//

#include "MenuArea.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "MenuBar.h"
#include "TraversalMenu.h"
#include "NodeMenu.h"

MenuArea::MenuArea(ApplicationContext& context)
    : ResizablePanel(context, ResizeEdge::Right, resizerWidth)
{
    menuBar = std::make_unique<MenuBar>(context);
    menuBar->traversalIcon->onClick = [this] { togglePanel(ActivePanel::Traversal); };
    menuBar->nodeIcon->onClick      = [this] { togglePanel(ActivePanel::Node); };

    traversalMenu = std::make_unique<TraversalMenu>(context, false);
    nodeMenu      = std::make_unique<NodeMenu>(context);

    addAndMakeVisible(menuBar.get());
    addChildComponent(traversalMenu.get());
    addChildComponent(nodeMenu.get());
}

MenuArea::~MenuArea() = default;

void MenuArea::paint(juce::Graphics &g) {
    ResizablePanel::paint(g);

    auto bounds = getLocalBounds().toFloat();
    auto barHeight = std::floor(bounds.getHeight() * 0.05f);

    drawTopBar(g, bounds.withHeight(barHeight).withTrimmedRight((float) resizerWidth));
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
