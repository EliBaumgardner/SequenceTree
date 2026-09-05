//
// Created by Eli Baumgardner on 7/17/26.
//

#ifndef SEQUENCETREE_MENUAREA_H
#define SEQUENCETREE_MENUAREA_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "../Bar.h"
#include "../PanelResizer.h"

class MenuBar;
class TraversalMenu;
class NodeMenu;

class MenuArea : public juce::Component {

public:

    explicit MenuArea(ApplicationContext& context);
    ~MenuArea() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    static constexpr int resizerWidth = 10;
    static constexpr int menuBarWidth = 28;
    static constexpr int minMenuWidth = resizerWidth + menuBarWidth;

    PanelResizer resizer;

private:

    enum class ActivePanel { None, Traversal, Node };

    void togglePanel(ActivePanel panel);

    Bar topBar;

    std::unique_ptr<MenuBar> menuBar = nullptr;
    std::unique_ptr<TraversalMenu> traversalMenu = nullptr;
    std::unique_ptr<NodeMenu> nodeMenu = nullptr;

    ActivePanel activePanel = ActivePanel::None;
};

#endif //SEQUENCETREE_MENUAREA_H
