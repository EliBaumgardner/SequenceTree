#include "OrnamentCanvas.h"
#include "../Node/Arrow.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Input/FigureDocument.h"
#include "../../Graph/OrnamentLibraries.h"
#include "../../Graph/OrnamentFigure.h"
#include "../../Graph/ValueTreeIdentifiers.h"

OrnamentCanvas::OrnamentCanvas(FigureDocument& document, juce::ValueTree canvasSettings)
    : NodeArrowCanvas(document),
      gridMultiplier (*this, canvasSettings, ValueTreeIdentifiers::OrnamentGridMultiplier)
{
    gridDisplay = GridDisplay::Always;

    updateFrames = juce::VBlankAttachment(this, [this](double) {
        if (rebuildRequested) {
            rebuildRequested = false;

            rebuildFromNodeMap(this->document.nodes);
        }

        if (! asyncUpdates.empty()) {
            handleAsyncUpdate();
        }
    });
}

void OrnamentCanvas::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Rectangle<float> visible = getLocalBounds().toFloat().transformedBy(modelTransform);

    const float lineWidth = 1.0f / viewTransform.getScaleFactor();

    graphics.setColour(theme.canvasColour);
    graphics.fillRect(getLocalBounds());

    graphics.addTransform(viewTransform);

    graphics.setColour(theme.gridColour);

    for (float x = std::floor(visible.getX() / gridSpacing) * gridSpacing; x < visible.getRight(); x += gridSpacing) {
        graphics.drawLine(x, visible.getY(), x, visible.getBottom(), lineWidth);
    }

    for (float y = std::floor(visible.getY() / gridSpacing) * gridSpacing; y < visible.getBottom(); y += gridSpacing) {
        graphics.drawLine(visible.getX(), y, visible.getRight(), y, lineWidth);
    }

    graphics.setColour(theme.borderStrongColour);
    graphics.drawLine(visible.getX(), 0.0f, visible.getRight(), 0.0f, lineWidth);
}

void OrnamentCanvas::rebuildFromNodeMap(const juce::ValueTree& stateTree)
{
    ArsNovaUI::EditCanvas::rebuildFromNodeMap(stateTree);

    gridOrigin    = { 0.0f, 0.0f };
    gridOriginSet = true;
    gridVisible   = true;
}

void OrnamentCanvas::nodeCreated(ArsNovaUI::CanvasNode& canvasNode)
{
    Node& node = static_cast<Node&>(canvasNode);

    juce::ValueTree figure = document.nodes;

    const juce::CachedValue<int> storedHostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, OrnamentFigure::noHostId);

    const int hostId = storedHostId;

    node.nodeColour  = ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour;
    node.hasInnerRim = node.nodeId == hostId;

    node.bindToTree();
    node.setDisplayMode(NodeInfo::DisplayMode::Pitch);
}

void OrnamentCanvas::prepareArrow(ArsNovaUI::CanvasArrow& arrow)
{
    static_cast<Arrow&>(arrow).valueEditor->setVisible(false);
}

void OrnamentCanvas::refreshHost()
{
    juce::ValueTree figure = document.nodes;

    const juce::CachedValue<int> storedHostId(figure, ValueTreeIdentifiers::OrnamentHostId, nullptr, OrnamentFigure::noHostId);

    const int hostId = storedHostId;

    for (const auto& [nodeId, node] : nodeManager.all()) {
        Node* const figureNode = static_cast<Node*>(node.get());

        if (figureNode->hasInnerRim != (nodeId == hostId)) {
            figureNode->hasInnerRim = nodeId == hostId;

            figureNode->repaint();
        }
    }
}

std::unique_ptr<ArsNovaUI::CanvasNode> OrnamentCanvas::makeNode(const juce::ValueTree&)
{
    return std::make_unique<OrnamentNode>(document.undoManager);
}