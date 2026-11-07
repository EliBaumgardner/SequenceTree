#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/PanelResizer.h>

/*
    The panel along the side of the editor that holds the MenuBar of icons and the two menus the
    icons open. It lays out a bar across the top of the menu column, a resizer on its inner edge,
    and the TraversalMenu and NodeMenu in the space beside the MenuBar. It draws frosted glass
    over the NodeCanvas behind them.

    Clicking the traversal icon or the node icon calls togglePanel, which shows that menu and
    hides the other, or hides it when it was already shown, and marks the icon as selected.
    narrowestHeight is the least height that fits the icons and both menus, and the resizer's
    onWidthDragged is how PluginEditor changes the panel's width.

    PluginEditor owns it, and owns the two menus it hands it.
*/

class MenuBar;
class NodeCanvas;
class TraversalMenu;
class NodeMenu;

class MenuArea : public juce::Component
{
public:

    MenuArea(NodeCanvas& nodeCanvas, TraversalMenu& traversalMenu, NodeMenu& nodeMenu);

    ~MenuArea() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    int narrowestHeight() const;

    static constexpr float resizerWidthRatio = 0.256f;
    static constexpr float menuBarWidthRatio = 0.896f;

    ArsNovaUI::PanelResizer resizer;

private:

    enum class ActivePanel;

    void togglePanel(ActivePanel panel);

    enum class ActivePanel { None, Traversal, Node };

    ArsNovaUI::Bar topBar;

    NodeCanvas&    nodeCanvas;
    TraversalMenu& traversalMenu;
    NodeMenu&      nodeMenu;

    std::unique_ptr<MenuBar> menuBar;

    ActivePanel activePanel = ActivePanel::None;
};
