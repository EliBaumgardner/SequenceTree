#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct ApplicationContext;

class PanelResizer : public juce::Component
{
public:

    enum class Edge { Left, Right };

    PanelResizer(const ApplicationContext& context, Edge edge);
    ~PanelResizer() override;

    void paint(juce::Graphics& graphics) override;

    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;
    void mouseEnter(const juce::MouseEvent& event) override;
    void mouseExit(const juce::MouseEvent& event) override;

    std::function<void(int)> onWidthDragged;

private:

    const Edge edge;

    int  dragStartWidth = 0;
    bool isHovered      = false;
    bool isDragging     = false;
};
