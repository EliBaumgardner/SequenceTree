#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>

/*
    The vertical bar of three icons at the side of the editor: a tree icon (disabled), a node icon
    and a traversal icon. It is a Bar drawn as frosted glass over the NodeCanvas, and it stacks
    the icons as squares the width of the bar. paintOverBar draws an accent marker beside the node
    or traversal icon while that icon is selected.

    The bar only holds and lays out the buttons. MenuArea owns it and decides what clicking the
    node and traversal icons shows.
*/

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
