#include "MenuArea.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../Bars/MenuBar.h"
#include "TraversalMenu.h"
#include "NodeMenu.h"

MenuArea::MenuArea(NodeCanvas& nodeCanvas, GraphState& graphState, TraversalRuleState& traversalRuleState, AudioSnapshotPublisher& snapshots,
                   juce::PropertiesFile& interfaceSettings, juce::ValueTree colourPresets, juce::UndoManager& undoManager)
    : resizer(PanelResizer::Edge::Right),
      topBar(nodeCanvas, Bar::Orientation::Horizontal, Theme::contentInsetRatio),
      nodeCanvas(nodeCanvas),
      interfaceSettings(interfaceSettings),
      colourPresets(colourPresets),
      undoManager(undoManager)
{
    menuBar       = std::make_unique<MenuBar>(nodeCanvas);
    traversalMenu = std::make_unique<TraversalMenu>(nodeCanvas, graphState, traversalRuleState, snapshots, colourPresets, undoManager);
    nodeMenu      = std::make_unique<NodeMenu>(nodeCanvas, graphState, traversalRuleState, snapshots, colourPresets, undoManager);

    menuBar->traversalIcon.onClick = [this] { togglePanel(ActivePanel::Traversal); };
    menuBar->nodeIcon.onClick      = [this] { togglePanel(ActivePanel::Node); };

    settingsPane.addButton(&CustomLookAndFeel::drawSettingsIcon, "Settings", [this]() { settingsLauncher.show(); });

    addAndMakeVisible(topBar);
    addAndMakeVisible(settingsPane);
    addAndMakeVisible(menuBar.get());

    addChildComponent(traversalMenu.get());
    addChildComponent(nodeMenu.get());

    addAndMakeVisible(resizer);
}

MenuArea::~MenuArea() = default;

void MenuArea::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);

    lookAndFeel.drawFrostedGlass(graphics, *this, nodeCanvas, resizer.getBounds());
    lookAndFeel.drawFrostedGlass(graphics, *this, nodeCanvas, menuBar->getBounds().withTop(0).withBottom(menuBar->getY()));
    lookAndFeel.drawFrostedGlass(graphics, *this, nodeCanvas, getLocalBounds().withRight(menuBar->getX()).withTrimmedTop(topBar.getBottom()));
}

void MenuArea::resized()
{
    const int barHeight = static_cast<int>(getHeight() * Theme::barHeightRatio);
    const int barInset  = juce::roundToInt(barHeight * Theme::contentInsetRatio);
    auto      bounds    = getLocalBounds();

    resizer.setBounds(bounds.removeFromRight(juce::roundToInt(barHeight * resizerWidthRatio)));

    auto menuColumn  = bounds.removeFromRight(juce::roundToInt(barHeight * menuBarWidthRatio));
    auto settingsRow = menuColumn.removeFromTop(barHeight + Theme::windowGutter).withTrimmedBottom(Theme::windowGutter).reduced(barInset);

    settingsPane.setBounds(settingsRow.withSizeKeepingCentre(settingsPane.idealWidth(settingsRow.getHeight()), settingsRow.getHeight()));
    menuBar->setBounds(menuColumn);
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
