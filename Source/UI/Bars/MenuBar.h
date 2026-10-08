#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"

class MenuBar : public Bar
{
public:

    IconButton treeIcon;
    IconButton nodeIcon;
    IconButton traversalIcon;

    explicit MenuBar(NodeCanvas& nodeCanvas);

private:

    void paintOverBar(juce::Graphics& graphics) override;
    void resized() override;

    static constexpr float iconInsetRatio = 0.214f;
};
