//
// Created by Eli Baumgardner on 7/17/26.
//

#include "MenuArea.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Bars/MenuBar.h"
#include "TraversalMenu.h"
#include "NodeMenu.h"

MenuArea::MenuArea(const ApplicationContext& context)
    : resizer(context, PanelResizer::Edge::Right),
      topBar(context, { Bar::Orientation::horizontal })
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
    auto bounds    = getLocalBounds();
    int  barHeight = static_cast<int>(getHeight() * Theme::barHeightRatio);

    resizer.setBounds(bounds.removeFromRight(juce::roundToInt(barHeight * resizerWidthRatio)));
    menuBar->setBounds(bounds.removeFromRight(juce::roundToInt(barHeight * menuBarWidthRatio)));

    topBar.setBounds(bounds.withHeight(barHeight));

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
