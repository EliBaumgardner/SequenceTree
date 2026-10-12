#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"

class NodeCanvas;

class MenuBar : public Bar
{
public:

    explicit MenuBar(NodeCanvas& nodeCanvas);

    IconButton treeIcon;
    IconButton nodeIcon;
    IconButton traversalIcon;

    static constexpr float iconInsetRatio = 0.214f;

private:

    void paintOverBar(juce::Graphics& graphics) override;
    void resized() override;
};
