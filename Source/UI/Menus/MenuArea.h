#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Bars/Bar.h"
#include "../PanelResizer.h"

class MenuBar;
class TraversalMenu;
class NodeMenu;

class MenuArea : public juce::Component
{
public:

    explicit MenuArea(const ApplicationContext& context);
    ~MenuArea() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    static constexpr float resizerWidthRatio = 0.4f;
    static constexpr float menuBarWidthRatio = 1.12f;

    PanelResizer resizer;

private:

    enum class ActivePanel { None, Traversal, Node };

    void togglePanel(ActivePanel panel);

    Bar topBar;

    std::unique_ptr<MenuBar>       menuBar;
    std::unique_ptr<TraversalMenu> traversalMenu;
    std::unique_ptr<NodeMenu>      nodeMenu;

    ActivePanel activePanel = ActivePanel::None;
};
