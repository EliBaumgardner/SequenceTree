#include "FigureDocument.h"
#include "../Graph/OrnamentLibraries.h"
#include "../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

FigureDocument::FigureDocument(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : ArsNovaUI::CanvasDocument({}, undoManager), ornamentLibraries(ornamentLibraries)
{
}

juce::ValueTree FigureDocument::findNode(int nodeId) const
{
    return nodes.getChildWithProperty(ArsNovaUI::CanvasIdentifiers::Id, nodeId);
}

juce::ValueTree FigureDocument::addChildNode(int parentNodeId, juce::Point<int> position, const juce::ModifierKeys&)
{
    return ornamentLibraries.addFigureNode(ornament, parentNodeId, position, &undoManager);
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
    ornamentLibraries.addFigureNode(ornament, -1, position, &undoManager);
}

void FigureDocument::moveNode(juce::ValueTree nodeTree, juce::Point<int> position)
{
    ornamentLibraries.setFigureNodePosition(ornament, nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), position, &undoManager);
}

void FigureDocument::removeNode(int nodeId)
{
    ornamentLibraries.removeFigureNode(ornament, nodeId, &undoManager);
}

void FigureDocument::addDanglingArrow(juce::ValueTree nodeTree, juce::Point<int> tipOffset)
{
    ornamentLibraries.addDanglingArrow(ornament, nodeTree.getProperty(ArsNovaUI::CanvasIdentifiers::Id), tipOffset, &undoManager);
}

void FigureDocument::setDanglingArrowTip(juce::ValueTree, juce::Point<int> tipOffset)
{
    ornamentLibraries.setLastNodeLength(ornament, tipOffset, &undoManager);
}

void FigureDocument::removeDanglingArrow(juce::ValueTree)
{
    ornamentLibraries.removeDanglingArrow(ornament, &undoManager);
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
