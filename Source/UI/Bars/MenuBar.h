#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>

/*
    The vertical bar of icons at the side of the editor: a settings icon, then, a group gap below
    it, a node icon and a traversal icon. A tree icon (disabled) is kept under the settings icon
    but not shown. It is a Bar drawn as frosted glass over the NodeCanvas, and it stacks the icons
    at the size of the Titlebar's buttons, turned to run down the bar. paintOverBar draws a bordered
    pane behind the settings icon and an accent marker beside the node or traversal icon while that
    icon is selected.

    The bar only holds and lays out the buttons. MenuArea owns it and decides what clicking the
    settings, node and traversal icons does.
*/

class NodeCanvas;

class MenuBar : public ArsNovaUI::Bar
{
public:

    explicit MenuBar(NodeCanvas& nodeCanvas);

    ArsNovaUI::IconButton treeIcon;
    ArsNovaUI::IconButton settingsIcon;
    ArsNovaUI::IconButton nodeIcon;
    ArsNovaUI::IconButton traversalIcon;

    static constexpr float iconInsetRatio = 0.167f;
    static constexpr float groupGapRatio    = 0.75f;
    static constexpr float markerInsetRatio = 0.22f;

private:

    void paintOverBar(juce::Graphics& graphics) override;
    void resized () override;
};
