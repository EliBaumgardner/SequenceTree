#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"

class MenuBar : public Bar
{
public:

    IconButton treeIcon;
    IconButton nodeIcon;
    IconButton traversalIcon;

    static constexpr float iconInsetRatio = 0.214f;

    explicit MenuBar(NodeCanvas& nodeCanvas);

private:

    void paintOverBar(juce::Graphics& graphics) override;
    void resized() override;
};
