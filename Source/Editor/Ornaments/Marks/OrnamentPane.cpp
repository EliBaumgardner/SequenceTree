#include "OrnamentPane.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Editor/Canvas/NodeCanvas.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Processor/Document/Ornaments/OrnamentSymbol.h"
#include "Processor/Document/TypedProperties.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

#include <cmath>

OrnamentPane::OrnamentPane(NodeCanvas& nodeCanvas, OrnamentLibraries& ornamentLibraries)
    : ornamentLibraries(ornamentLibraries)
{
    button = &addButton("Ornaments", [&nodeCanvas]() {
        if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Ornament) {
            nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);

            return;
        }

        nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::Ornament);
    });

    button->state.look   = ArsNovaUI::ButtonState::Look::Raised;
    button->onRightClick = [this]() { onWindowRequested(); };

    button->painter = [this](juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state) {
        paintGlyph(graphics, bounds, state);
    };

    ornamentLibraries.libraries.addListener(this);

    showActiveOrnamentName();
}

OrnamentPane::~OrnamentPane()
{
    ornamentLibraries.libraries.removeListener(this);
}

void OrnamentPane::paintGlyph(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ArsNovaUI::ButtonState& state)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Rectangle<float> glyphArea = lookAndFeel.drawButtonTile(graphics, bounds, state);

    const float cellSide      = glyphArea.getHeight() / gridCellsPerHeight;
    const float columnsToEdge = std::floor(glyphArea.getWidth() * 0.5f / cellSide);
    const float rowsToEdge    = std::floor(glyphArea.getHeight() * 0.5f / cellSide);
    const float firstColumn   = glyphArea.getCentreX() - columnsToEdge * cellSide;
    const float firstRow      = glyphArea.getCentreY() - rowsToEdge * cellSide;

    const juce::String activeUid = ornamentLibraries.activeOrnamentUid;

    const OrnamentProperties active(ornamentLibraries.findOrnament(activeUid));

    const juce::String symbol = active.symbol;

    const std::vector<ArsNovaUI::SymbolStroke> strokes = OrnamentSymbol::read(symbol);

    juce::Rectangle<float> extent;

    juce::Path tileOutline;

    tileOutline.addRoundedRectangle(glyphArea, ArsNovaUI::Theme::paneCornerRadius);

    graphics.saveState();
    graphics.reduceClipRegion(tileOutline);
    graphics.setColour(lookAndFeel.gridColour);

    for (float x = firstColumn; x < glyphArea.getRight(); x += cellSide) {
        graphics.drawLine(x, glyphArea.getY(), x, glyphArea.getBottom(), ArsNovaUI::Theme::borderThickness);
    }

    for (float y = firstRow; y < glyphArea.getBottom(); y += cellSide) {
        graphics.drawLine(glyphArea.getX(), y, glyphArea.getRight(), y, ArsNovaUI::Theme::borderThickness);
    }

    graphics.restoreState();

    for (const ArsNovaUI::SymbolStroke& stroke : strokes) {
        const float halfWidth = stroke.width * 0.5f;

        extent = extent.getUnion(stroke.path.getBounds().expanded(halfWidth));
    }

    if (extent.isEmpty()) {
        lookAndFeel.drawIcon(graphics, glyphArea, ArsNovaUI::IconLibrary::get().wave, state);

        return;
    }

    const float fillSide   = juce::jmin(glyphArea.getWidth(), glyphArea.getHeight());
    const float extentSide = juce::jmax(extent.getWidth(), extent.getHeight());
    const float unitSide   = fillSide / extentSide;

    const juce::Point<float> unitOrigin = glyphArea.getCentre() - extent.getCentre() * unitSide;

    const juce::Rectangle<float> unitSquare = juce::Rectangle<float>(unitSide, unitSide).withPosition(unitOrigin);

    graphics.setColour(lookAndFeel.pressableButtonColour(state));

    lookAndFeel.drawStrokeSymbol(graphics, unitSquare, strokes);
}

void OrnamentPane::showActiveOrnamentName()
{
    const juce::String activeUid = ornamentLibraries.activeOrnamentUid;

    const juce::ValueTree active = ornamentLibraries.findOrnament(activeUid);

    juce::String tooltip = "Ornaments";

    const OrnamentProperties activeInfo(active);

    if (active.isValid()) {
        tooltip = "Ornaments - " + activeInfo.ornamentName.get();
    }

    button->setTooltip(tooltip);
}

void OrnamentPane::valueTreePropertyChanged(juce::ValueTree&, const juce::Identifier& property)
{
    if (property == ValueTreeIdentifiers::ActiveOrnamentUid || property == ValueTreeIdentifiers::OrnamentName
        || property == ValueTreeIdentifiers::OrnamentSymbol) {
        showActiveOrnamentName();

        button->repaint();
    }
}
