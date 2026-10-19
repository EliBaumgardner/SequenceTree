#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>

class NodeCanvas;

class MenuBar : public ArsNovaUI::Bar
{
public:

    explicit MenuBar(NodeCanvas& nodeCanvas);

    ArsNovaUI::IconButton treeIcon;
    ArsNovaUI::IconButton nodeIcon;
    ArsNovaUI::IconButton traversalIcon;

    static constexpr float iconInsetRatio = 0.214f;

private:

    void paintOverBar(juce::Graphics& graphics) override;
    void resized() override;
};
