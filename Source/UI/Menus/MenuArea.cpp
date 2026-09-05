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
    : resizer(context, PanelResizer::Edge::Right),
      topBar(context, { Bar::Orientation::horizontal, Bar::Background::litFromTop })
{
    setLookAndFeel(context.lookAndFeel);

    menuBar = std::make_unique<MenuBar>(context);
    menuBar->traversalIcon->onClick = [this] { togglePanel(ActivePanel::Traversal); };
    menuBar->nodeIcon->onClick      = [this] { togglePanel(ActivePanel::Node); };

    traversalMenu = std::make_unique<TraversalMenu>(context);
    nodeMenu      = std::make_unique<NodeMenu>(context);

    addAndMakeVisible(topBar);
    addAndMakeVisible(menuBar.get());
    addChildComponent(traversalMenu.get());
    addChildComponent(nodeMenu.get());
    addAndMakeVisible(resizer);
}

MenuArea::~MenuArea() {
    setLookAndFeel(nullptr);
}

void MenuArea::paint(juce::Graphics &g) {
    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(theme.baseDarkColour2);
    g.fillRect(getLocalBounds());
}

void MenuArea::resized() {
    auto bounds = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(resizerWidth));
    menuBar->setBounds(bounds.removeFromRight(menuBarWidth));

    topBar.setBounds(bounds.withHeight(static_cast<int>(getHeight() * 0.05f)));

    traversalMenu->setBounds(bounds);
    nodeMenu->setBounds(bounds);
}

void MenuArea::togglePanel(ActivePanel panel) {
    if (activePanel == panel) {
        activePanel = ActivePanel::None;
    } else {
        activePanel = panel;
    }


    traversalMenu->setVisible(activePanel == ActivePanel::Traversal);
    nodeMenu->setVisible(activePanel == ActivePanel::Node);

    resized();
}
