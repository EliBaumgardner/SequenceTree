#include "NodeArrowCanvas.h"
#include "../Node/Arrow.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include <ArsNovaUI/Canvas/CanvasIdentifiers.h>

NodeArrowCanvas::NodeArrowCanvas(ArsNovaUI::CanvasDocument& document) : ArsNovaUI::EditCanvas(document)
{
}

void NodeArrowCanvas::treePropertyChanged(const juce::ValueTree& tree, const juce::Identifier& propertyIdentifier)
{
    if (propertyIdentifier == ValueTreeIdentifiers::Radius) {
        asyncUpdates.push_back({ .type = AsyncUpdateType::NodeMoved, .nodeId = tree.getProperty(ArsNovaUI::CanvasIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::MidiPitch
        || propertyIdentifier == ValueTreeIdentifiers::MidiVelocity) {
        asyncUpdates.push_back({ .type   = AsyncUpdateType::ValueChanged,
                                 .nodeId = tree.getParent().getParent().getProperty(ArsNovaUI::CanvasIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::NodeColour) {
        asyncUpdates.push_back({ .type = AsyncUpdateType::NodeColourChanged, .nodeId = tree.getProperty(ArsNovaUI::CanvasIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::OrnamentUid) {
        asyncUpdates.push_back({ .type = AsyncUpdateType::NodeMarkChanged, .nodeId = tree.getProperty(ArsNovaUI::CanvasIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowDuration) {
        asyncUpdates.push_back({ .type   = AsyncUpdateType::ArrowDurationChanged,
                                 .nodeId = tree.getParent().getParent().getProperty(ArsNovaUI::CanvasIdentifiers::Id) });
    }
    else if (propertyIdentifier == ValueTreeIdentifiers::ArrowType
        || propertyIdentifier == ValueTreeIdentifiers::ArrowSync
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYBinding
        || propertyIdentifier == ValueTreeIdentifiers::ArrowXMultiplier
        || propertyIdentifier == ValueTreeIdentifiers::ArrowYMultiplier) {
        asyncUpdates.push_back({ .type        = AsyncUpdateType::ArrowInfoChanged,
                                 .nodeId      = tree.getParent().getParent().getProperty(ArsNovaUI::CanvasIdentifiers::Id),
                                 .childNodeId = tree.getProperty(ArsNovaUI::CanvasIdentifiers::Id) });
    }
}

std::unique_ptr<ArsNovaUI::CanvasArrow> NodeArrowCanvas::makeArrow(ArsNovaUI::CanvasNode* startNode, ArsNovaUI::CanvasNode* endNode)
{
    return std::make_unique<Arrow>(startNode, endNode, document.undoManager);
}

std::unique_ptr<ArsNovaUI::CanvasArrow> NodeArrowCanvas::makeDanglingArrow(ArsNovaUI::CanvasNode* startNode, juce::Point<int> tipOffset)
{
    return std::make_unique<Arrow>(startNode, tipOffset, document.undoManager);
}
