#include "MenuArea.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../Bars/MenuBar.h"
#include "TraversalMenu.h"
#include "NodeMenu.h"

MenuArea::MenuArea(const ApplicationContext& context)
    : resizer(context, PanelResizer::Edge::Right),
      topBar(context, { Bar::Orientation::Horizontal, Theme::contentInsetRatio, Bar::Surface::Frosted }),
      applicationContext(context)
{
    setLookAndFeel(context.lookAndFeel);

    menuBar       = std::make_unique<MenuBar>(context);
    traversalMenu = std::make_unique<TraversalMenu>(context);
    nodeMenu      = std::make_unique<NodeMenu>(context);

    menuBar->traversalIcon.onClick = [this] { togglePanel(ActivePanel::Traversal); };
    menuBar->nodeIcon.onClick      = [this] { togglePanel(ActivePanel::Node); };

    addAndMakeVisible(topBar);
    addAndMakeVisible(menuBar.get());

    addChildComponent(traversalMenu.get());
    addChildComponent(nodeMenu.get());

    addAndMakeVisible(resizer);
}

MenuArea::~MenuArea()
{
    setLookAndFeel(nullptr);
}

void MenuArea::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);
    juce::Component&   backdrop    = *applicationContext.canvas->getParentComponent();

    lookAndFeel.drawFrostedGlass(graphics, *this, backdrop, resizer.getBounds());
    lookAndFeel.drawFrostedGlass(graphics, *this, backdrop, getLocalBounds().withRight(menuBar->getX()).withTrimmedTop(topBar.getBottom()));
}

void MenuArea::resized()
{
    const int barHeight = static_cast<int>(getHeight() * Theme::barHeightRatio);
    auto      bounds    = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(juce::roundToInt(barHeight * resizerWidthRatio)));
    menuBar->setBounds(bounds.removeFromRight(juce::roundToInt(barHeight * menuBarWidthRatio)));
    topBar.setBounds(bounds.withHeight(barHeight));
    traversalMenu->setBounds(bounds);
    nodeMenu->setBounds(bounds);
}

void MenuArea::togglePanel(ActivePanel panel)
{
    if (activePanel == panel) {
        activePanel = ActivePanel::None;
    }
    else {
        activePanel = panel;
    }

    traversalMenu->setVisible(activePanel == ActivePanel::Traversal);
    nodeMenu->setVisible(activePanel == ActivePanel::Node);

    menuBar->traversalIcon.setSelected(activePanel == ActivePanel::Traversal);
    menuBar->nodeIcon.setSelected(activePanel == ActivePanel::Node);

    resized();

    menuBar->repaint();
}
