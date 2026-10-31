#include "OrnamentCanvas.h"
#include "../Node/Arrow.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Input/FigureDocument.h"
#include "../../Graph/OrnamentLibraries.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/ArrowInfo.h"

OrnamentCanvas::OrnamentCanvas(FigureDocument& document)
    : EditCanvas(document)
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
    const ArsNovaUI::Theme& theme     = ArsNovaUI::CustomLookAndFeel::get(*this);
    const float             grid      = ArrowInfo::pixelsPerGridSpace;
    const auto              visible   = getLocalBounds().toFloat().transformedBy(modelTransform);
    const float             lineWidth = 1.0f / viewTransform.getScaleFactor();

    graphics.setColour(theme.canvasColour);
    graphics.fillRect(getLocalBounds());

    graphics.addTransform(viewTransform);

    graphics.setColour(theme.gridColour);

    for (float x = std::floor(visible.getX() / grid) * grid; x < visible.getRight(); x += grid) {
        graphics.drawLine(x, visible.getY(), x, visible.getBottom(), lineWidth);
    }

    for (float y = std::floor(visible.getY() / grid) * grid; y < visible.getBottom(); y += grid) {
        graphics.drawLine(visible.getX(), y, visible.getRight(), y, lineWidth);
    }

    graphics.setColour(theme.borderStrongColour);
    graphics.drawLine(visible.getX(), 0.0f, visible.getRight(), 0.0f, lineWidth);
}

void OrnamentCanvas::rebuildFromNodeMap(const juce::ValueTree& stateTree)
{
    EditCanvas::rebuildFromNodeMap(stateTree);

    gridOrigin    = { 0.0f, 0.0f };
    gridSpacing   = ArrowInfo::pixelsPerGridSpace;
    gridOriginSet = true;
    gridVisible   = true;
}

void OrnamentCanvas::nodeCreated(Node& node)
{
    const int hostId = document.nodes.getProperty(ValueTreeIdentifiers::OrnamentHostId, OrnamentLibraries::noHostId);

    node.nodeColour  = ArsNovaUI::CustomLookAndFeel::get(*this).defaultNodeColour;
    node.hasInnerRim = node.nodeId == hostId;

    node.bindToTree();
    node.setDisplayMode(NodeInfo::DisplayMode::Pitch);
}

void OrnamentCanvas::prepareArrow(Arrow& arrow)
{
    arrow.valueEditor->setVisible(false);
}

void OrnamentCanvas::refreshHost()
{
    const int hostId = document.nodes.getProperty(ValueTreeIdentifiers::OrnamentHostId, OrnamentLibraries::noHostId);

    for (const auto& [nodeId, node] : nodeManager.all()) {
        if (node->hasInnerRim != (nodeId == hostId)) {
            node->hasInnerRim = nodeId == hostId;

            node->repaint();
        }
    }
}

std::unique_ptr<Node> OrnamentCanvas::makeNode(const juce::ValueTree&)
{
    return std::make_unique<OrnamentNode>(document.undoManager);
}