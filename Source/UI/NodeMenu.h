//
// Created by Eli Baumgardner on 7/17/26.
//

#ifndef SEQUENCETREE_NODEMENU_H
#define SEQUENCETREE_NODEMENU_H

#include <juce_gui_basics/juce_gui_basics.h>

struct ApplicationContext;

class NodeMenu : public juce::Component {

public:

    NodeMenu(ApplicationContext& context);
    ~NodeMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int)> onWidthDragged;

    static constexpr int resizerWidth = 15;
    static constexpr int minMenuWidth = resizerWidth;

private:

    class Resizer : public juce::Component {
    public:
        explicit Resizer(NodeMenu& owner);

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void paint(juce::Graphics& g) override;

    private:
        NodeMenu& owner;
        int dragStartWidth = 0;
        int dragStartX = 0;
        bool isHovered = false;
        bool isDragging = false;
    };

    Resizer resizer { *this };
};

#endif //SEQUENCETREE_NODEMENU_H
