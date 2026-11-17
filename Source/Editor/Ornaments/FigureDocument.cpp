#include "FigureDocument.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentFigure.h"
#include "Processor/Document/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

FigureDocument::FigureDocument(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : ArsNovaUI::CanvasDocument({}, undoManager),
      ornamentLibraries        (ornamentLibraries)
{
}

juce::ValueTree FigureDocument::findNode(int nodeId) const
{
    return nodes.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
}

juce::ValueTree FigureDocument::addChildNode(int parentNodeId, juce::Point<int> position, const juce::ModifierKeys&)
{
    return OrnamentFigure::addNode(ornament, parentNodeId, position, &undoManager);
}

int FigureDocument::growFrom(int nodeId) const
{
    const juce::ValueTree nodeTree = findNode(nodeId);

    if (! nodeTree.isValid() || nodeTree.getChildWithName(ArsNovaUI::CanvasIdentifiers::NodeChildrenIds).getNumChildren() > 0) {
        return -1;
    }

    return nodeId;
}

void FigureDocument::addRootNode(juce::Point<int> position)
{
    OrnamentFigure::addNode(ornament, -1, position, &undoManager);
}

void FigureDocument::moveNode(juce::ValueTree nodeTree, juce::Point<int> position)
{
    OrnamentFigure::setNodePosition(ornament, nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), position, &undoManager);
}

void FigureDocument::removeNode(int nodeId)
{
    OrnamentFigure::removeNode(ornament, nodeId, &undoManager);
}

void FigureDocument::addDanglingArrow(juce::ValueTree nodeTree, juce::Point<int> tipOffset)
{
    OrnamentFigure::addDanglingArrow(ornament, nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), tipOffset, &undoManager);
}

void FigureDocument::setDanglingArrowTip(juce::ValueTree, juce::Point<int> tipOffset)
{
    OrnamentFigure::setLastNodeLength(ornament, tipOffset, &undoManager);
}

void FigureDocument::removeDanglingArrow(juce::ValueTree)
{
    OrnamentFigure::removeDanglingArrow(ornament, &undoManager);
}

void FigureDocument::connect(int, int, ConnectionGesture)
{
}

void FigureDocument::disconnect(int, int)
{
}

bool FigureDocument::canConnect(const ArsNovaUI::CanvasNode&, const ArsNovaUI::CanvasNode&) const
{
    return false;
}
