#pragma once

#include <ArsNovaUI/Canvas/EditCanvas.h>
#include <ArsNovaUI/Canvas/CanvasDocument.h>
#include <ArsNovaUI/Canvas/CanvasNode.h>
#include <ArsNovaUI/Canvas/CanvasArrow.h>

/*
    The ArsNovaUI::EditCanvas whose components are Node and Arrow, shared by the graph's canvas
    (NodeCanvas) and the ornament figure's (OrnamentCanvas). It makes every arrow an Arrow, and
    turns a change to a property the library's tree listener does not know into the AsyncUpdate
    that redraws what it changed: a node's radius, a note's pitch or velocity, a node's colour or
    ornament, an arrow's duration, and an arrow's type, sync, bindings or multipliers. Which Node
    class a tree becomes is left to each subclass.
*/

class NodeArrowCanvas : public ArsNovaUI::EditCanvas
{
public:

    explicit NodeArrowCanvas(ArsNovaUI::CanvasDocument& document);

    void treePropertyChanged(const juce::ValueTree& tree, const juce::Identifier& propertyIdentifier) override;

    std::unique_ptr<ArsNovaUI::CanvasArrow> makeArrow        (ArsNovaUI::CanvasNode* startNode, ArsNovaUI::CanvasNode* endNode) override;
    std::unique_ptr<ArsNovaUI::CanvasArrow> makeDanglingArrow(ArsNovaUI::CanvasNode* startNode, juce::Point<int> tipOffset) override;
};
