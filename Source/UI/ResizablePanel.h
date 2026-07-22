//
// Created by Eli Baumgardner on 7/21/26.
//

#ifndef SEQUENCETREE_RESIZABLEPANEL_H
#define SEQUENCETREE_RESIZABLEPANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

struct ApplicationContext;

class ResizablePanel : public juce::Component {

public:

    enum class ResizeEdge { Left, Right };

    ResizablePanel(ApplicationContext& context, ResizeEdge edge, int resizerThickness, bool showResizer = true);
    ~ResizablePanel() override;

    void paint(juce::Graphics& g) override;

    bool hasResizer() const { return resizerVisible; }

    std::function<void(int)> onWidthDragged;

protected:

    virtual int  resizeStartWidth() const;
    virtual int  minimumWidth() const;
    virtual void applyResizedWidth(int newWidth);

    void drawTopBar(juce::Graphics& g, juce::Rectangle<float> barBounds);

    class Resizer : public juce::Component {
    public:
        explicit Resizer(ResizablePanel& owner);

        void mouseDown (const juce::MouseEvent& e) override;
        void mouseDrag (const juce::MouseEvent& e) override;
        void mouseUp   (const juce::MouseEvent& e) override;
        void mouseEnter(const juce::MouseEvent& e) override;
        void mouseExit (const juce::MouseEvent& e) override;
        void paint(juce::Graphics& g) override;

    private:
        ResizablePanel& owner;
        bool isHovered  = false;
        bool isDragging = false;
    };

    ApplicationContext& applicationContext;

    int dragStartWidth = 0;
    int dragStartX     = 0;

    const int  resizerThickness;
    const bool resizerVisible;

    Resizer resizer { *this };

private:

    void beginResize();
    void dragResize(int deltaX);

    const ResizeEdge edge;
};

#endif //SEQUENCETREE_RESIZABLEPANEL_H
