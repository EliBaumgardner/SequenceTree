//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_PANELRESIZER_H
#define SEQUENCETREE_PANELRESIZER_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct ApplicationContext;

class PanelResizer : public juce::Component {

public:

    enum class Edge { Left, Right };

    PanelResizer(ApplicationContext& context, Edge edge);
    ~PanelResizer() override;

    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp   (const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit (const juce::MouseEvent& e) override;

    void paint(juce::Graphics& g) override;

    std::function<void(int)> onWidthDragged;

private:

    const Edge edge;

    int  dragStartWidth = 0;
    bool isHovered      = false;
    bool isDragging     = false;
};

#endif //SEQUENCETREE_PANELRESIZER_H
