//
// Created by Eli Baumgardner on 5/23/26.
//

#ifndef SEQUENCETREE_TRAVERSALMENU_H
#define SEQUENCETREE_TRAVERSALMENU_H

#include <juce_gui_basics/juce_gui_basics.h>

#include "TraversalDisplayMenu.h"

class TraversalMenu : public juce::Component {

public:

    TraversalMenu(ApplicationContext& context);
    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void(int)> onWidthDragged;

    static constexpr int resizerWidth = 6;
    static constexpr int minMenuWidth = 60;

private:

    class Resizer : public juce::Component {
    public:
        explicit Resizer(TraversalMenu& owner);

        void mouseDown(const juce::MouseEvent& e) override;
        void mouseDrag(const juce::MouseEvent& e) override;
        void mouseUp(const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit(const juce::MouseEvent& e) override;
        void paint(juce::Graphics& g) override;

    private:
        TraversalMenu& owner;
        int dragStartWidth = 0;
        int dragStartX = 0;
        bool isHovered = false;
        bool isDragging = false;
    };

    TraversalDisplayMenu displayMenu;
    Resizer resizer { *this };
};

#endif //SEQUENCETREE_TRAVERSALMENU_H
