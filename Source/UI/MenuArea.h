//
// Created by Eli Baumgardner on 7/17/26.
//

#ifndef SEQUENCETREE_MENUAREA_H
#define SEQUENCETREE_MENUAREA_H

#include <juce_gui_basics/juce_gui_basics.h>

struct ApplicationContext;

class MenuBar;
class TraversalMenu;
class NodeMenu;

class MenuArea : public juce::Component {

public:

    MenuArea(ApplicationContext& context);
    ~MenuArea() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int)> onWidthDragged;

    static constexpr int resizerWidth = 10;
    static constexpr int menuBarWidth = 28;
    static constexpr int minMenuWidth = resizerWidth;

private:

    enum class ActivePanel { None, Traversal, Node };

    void togglePanel(ActivePanel panel);

    class Resizer : public juce::Component {
    public:
        explicit Resizer(MenuArea& owner);

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void paint(juce::Graphics& g) override;

    private:
        MenuArea& owner;
        int dragStartWidth = 0;
        int dragStartX = 0;
        bool isHovered = false;
        bool isDragging = false;
    };

    Resizer resizer { *this };

    std::unique_ptr<MenuBar> menuBar = nullptr;
    std::unique_ptr<TraversalMenu> traversalMenu = nullptr;
    std::unique_ptr<NodeMenu> nodeMenu = nullptr;

    ActivePanel activePanel = ActivePanel::None;
};

#endif //SEQUENCETREE_MENUAREA_H
