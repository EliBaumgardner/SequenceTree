#include "ValueField.h"
#include "NodeCanvas.h"
#include "Editor/Nodes/Node.h"
#include "Processor/Document/GraphState.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

ValueField::ValueField(NodeCanvas& nodeCanvas) : nodeCanvas(nodeCanvas)
{
    for (size_t step = 0; step < falloffTable.size(); ++step) {
        const float stepRatio = static_cast<float>(step) / static_cast<float>(falloffSteps);

        falloffTable[step] = std::pow(1.0f - stepRatio, falloffExponent);
    }
}

ValueField::~ValueField()
{
    stopTimer();
}

void ValueField::setActivePaintLayer(PaintLayer layer)
{
    activePaintLayer = layer;

    if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
        render();
    }

    nodeCanvas.repaint();
}

void ValueField::updateBrushCursor()
{
    if (nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint) {
        return;
    }

    const int brushDiameter = static_cast<int>(brushRadius * viewZoom * 2.0f);
    const int cursorSize    = juce::jmax(1, brushDiameter);
    const int hotspot       = cursorSize / 2;

    juce::Image cursorImage(juce::Image::ARGB, cursorSize, cursorSize, true);

    juce::Graphics cursorGraphics(cursorImage);

    cursorGraphics.setColour(juce::Colours::black);
    cursorGraphics.drawEllipse(cursorImage.getBounds().toFloat().reduced(1.0f), 1.0f);

    nodeCanvas.setMouseCursor(juce::MouseCursor(cursorImage, hotspot, hotspot));
}

void ValueField::refreshAround(int nodeId)
{
    const ArsNovaUI::CanvasNode* const node = nodeCanvas.nodeManager.find(nodeId);

    if (nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint || node == nullptr || !image.isValid()) {
        return;
    }

    const juce::Point<float> nodeCentre = node->getBounds().getCentre().toFloat();

    const juce::Rectangle<float> graphArea = juce::Rectangle<float>(nodeCentre, nodeCentre).expanded(glowRadius);
    const juce::Rectangle<int>   area      = graphArea.transformedBy(graphToImage).getSmallestIntegerContainer().expanded(1).getIntersection(image.getBounds());

    if (area.isEmpty()) {
        return;
    }

    renderArea(area);

    nodeCanvas.repaint(graphArea.transformedBy(nodeCanvas.viewTransform).getSmallestIntegerContainer());
}

void ValueField::refresh()
{
    if (nodeCanvas.activeTool != NodeCanvas::CanvasTool::Paint) {
        return;
    }

    render();

    nodeCanvas.repaint();
}

void ValueField::paintStroke(juce::Point<float> canvasPosition, bool isStart, bool erase)
{
    if (isStart) {
        brushStroke = BrushStroke::Painting;

        if (erase) {
            brushStroke = BrushStroke::Erasing;
        }

        ensurePaintBuffers();

        std::ranges::fill(strokeMask, 0.0f);

        seedStrokeDensityFromNodes();

        strokePreviousPoint = canvasPosition;

        startTimerHz(dwellTimerHz);
    }

    brushCurrentPoint = canvasPosition;

    accumulateStroke(strokePreviousPoint, canvasPosition);
    applyPaintToNodes(strokePreviousPoint, canvasPosition);

    strokePreviousPoint = canvasPosition;
}

void ValueField::endStroke()
{
    if (brushStroke == BrushStroke::Idle) {
        return;
    }

    brushStroke = BrushStroke::Idle;

    stopTimer();
}

void ValueField::render()
{
    juce::Rectangle<float> graphArea;

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const juce::Point<float> nodeCentre = node->getBounds().getCentre().toFloat();

        graphArea = graphArea.getUnion(juce::Rectangle<float>(nodeCentre, nodeCentre).expanded(glowRadius));
    }

    if (graphArea.isEmpty()) {
        image = juce::Image();

        return;
    }

    const float longestSide  = juce::jmax(graphArea.getWidth(), graphArea.getHeight());
    const float sideCellSize = longestSide / static_cast<float>(maximumFieldSide);

    fieldCellSize = juce::jmax(fieldUnitsPerPixel, sideCellSize);

    const int widthCells  = static_cast<int>(std::ceil(graphArea.getWidth()  / fieldCellSize));
    const int heightCells = static_cast<int>(std::ceil(graphArea.getHeight() / fieldCellSize));
    const int fieldWidth  = juce::jmax(1, widthCells);
    const int fieldHeight = juce::jmax(1, heightCells);

    const size_t cellCount = static_cast<size_t>(fieldWidth) * static_cast<size_t>(fieldHeight);

    imageToGraph = juce::AffineTransform::scale(fieldCellSize).translated(graphArea.getPosition());
    graphToImage = imageToGraph.inverted();

    if (image.getWidth() != fieldWidth || image.getHeight() != fieldHeight) {
        image = juce::Image(juce::Image::ARGB, fieldWidth, fieldHeight, false);
    }

    fieldWeightedSum .resize(cellCount);
    fieldTotalWeight .resize(cellCount);
    fieldCoverageProd.resize(cellCount);

    for (size_t step = 0; step < colourTable.size(); ++step) {
        const float colourRatio = static_cast<float>(step) / static_cast<float>(colourSteps);

        colourTable[step] = mapFieldColour(colourRatio);
    }

    renderArea(image.getBounds());
}

void ValueField::renderArea(juce::Rectangle<int> area)
{
    const size_t fieldWidth = static_cast<size_t>(image.getWidth());

    accumulateNodeGlow(area);

    juce::Image::BitmapData pixels(image, area.getX(), area.getY(), area.getWidth(), area.getHeight(), juce::Image::BitmapData::writeOnly);

    for (int row = area.getY(); row < area.getBottom(); ++row) {
        const size_t rowStart = static_cast<size_t>(row) * fieldWidth;

        const int bitmapRow = row - area.getY();

        const float* weightedSum  = fieldWeightedSum.data()  + rowStart;
        const float* totalWeight  = fieldTotalWeight.data()  + rowStart;
        const float* coverageProd = fieldCoverageProd.data() + rowStart;
        juce::uint8* line         = pixels.getLinePointer(bitmapRow);

        for (int column = area.getX(); column < area.getRight(); ++column) {
            juce::Colour pixelColour = juce::Colours::transparentBlack;

            const int bitmapColumn = column - area.getX();

            juce::uint8* const pixelBytes = line + bitmapColumn * pixels.pixelStride;

            juce::PixelARGB* pixel = reinterpret_cast<juce::PixelARGB*>(pixelBytes);

            if (totalWeight[column] > 0.0f) {
                const float fieldFactor = juce::jlimit(0.0f, 1.0f, weightedSum[column] / totalWeight[column]);
                const float coverage    = juce::jlimit(0.0f, 1.0f, 1.0f - coverageProd[column]);

                const float colourPosition = fieldFactor * static_cast<float>(colourSteps);

                const size_t colourStep = static_cast<size_t>(std::round(colourPosition));

                pixelColour = colourTable[colourStep].withMultipliedAlpha(coverage);
            }

            pixel->set(pixelColour.getPixelARGB());
        }
    }
}

void ValueField::accumulateNodeGlow(juce::Rectangle<int> area)
{
    const juce::Identifier valueId = paintLayerValueId();

    const float falloffStepCount = static_cast<float>(falloffSteps);
    const float fieldRadius      = glowRadius / fieldCellSize;
    const float falloffScale     = falloffStepCount / fieldRadius;

    const int lastAreaColumn = area.getRight() - 1;
    const int lastAreaRow    = area.getBottom() - 1;

    const size_t fieldWidth = static_cast<size_t>(image.getWidth());
    const size_t areaWidth  = static_cast<size_t>(area.getWidth());

    for (int row = area.getY(); row < area.getBottom(); ++row) {
        const size_t areaStart = static_cast<size_t>(row) * fieldWidth + static_cast<size_t>(area.getX());

        float* const weightedSumStart  = fieldWeightedSum.data()  + areaStart;
        float* const totalWeightStart  = fieldTotalWeight.data()  + areaStart;
        float* const coverageProdStart = fieldCoverageProd.data() + areaStart;

        std::fill_n(weightedSumStart,  areaWidth, 0.0f);
        std::fill_n(totalWeightStart,  areaWidth, 0.0f);
        std::fill_n(coverageProdStart, areaWidth, 1.0f);
    }

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const juce::Point<float> nodeCentre = node->getBounds().getCentre().toFloat().transformedBy(graphToImage) - juce::Point<float>(0.5f, 0.5f);

        const int leftCell    = static_cast<int>(std::floor(nodeCentre.x - fieldRadius));
        const int rightCell   = static_cast<int>(std::ceil (nodeCentre.x + fieldRadius));
        const int topCell     = static_cast<int>(std::floor(nodeCentre.y - fieldRadius));
        const int bottomCell  = static_cast<int>(std::ceil (nodeCentre.y + fieldRadius));
        const int firstColumn = juce::jmax(area.getX(), leftCell);
        const int lastColumn  = juce::jmin(lastAreaColumn, rightCell);
        const int firstRow    = juce::jmax(area.getY(), topCell);
        const int lastRow     = juce::jmin(lastAreaRow, bottomCell);

        if (lastColumn < firstColumn || lastRow < firstRow) {
            continue;
        }

        const juce::ValueTree note = nodeCanvas.graphState.getMidiNotes(nodeId).getChild(0);

        if (!note.isValid()) {
            continue;
        }

        const int noteValue = note.getProperty(valueId);

        const float valueFactor = juce::jlimit(0.0f, 1.0f, noteValue / maximumMidiValue);

        for (int row = firstRow; row <= lastRow; ++row) {
            const float offsetY = static_cast<float>(row) - nodeCentre.y;

            const size_t rowStart = static_cast<size_t>(row) * fieldWidth;

            float* weightedSum  = fieldWeightedSum.data()  + rowStart;
            float* totalWeight  = fieldTotalWeight.data()  + rowStart;
            float* coverageProd = fieldCoverageProd.data() + rowStart;

            for (int column = firstColumn; column <= lastColumn; ++column) {
                const float offsetX        = static_cast<float>(column) - nodeCentre.x;
                const float distanceSquare = offsetX * offsetX + offsetY * offsetY;
                const float falloffPosition = juce::jmin(falloffStepCount, std::sqrt(distanceSquare) * falloffScale);
                const float weight          = falloffTable[static_cast<size_t>(falloffPosition)];

                weightedSum[column]  += weight * valueFactor;
                totalWeight[column]  += weight;
                coverageProd[column] *= (1.0f - weight);
            }
        }
    }
}

void ValueField::accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm)
{
    const int   canvasWidth     = nodeCanvas.getWidth();
    const int   canvasHeight    = nodeCanvas.getHeight();
    const float viewBrushRadius = brushRadius * viewZoom;

    if (canvasWidth <= 0 || canvasHeight <= 0) {
        return;
    }

    ensurePaintBuffers();

    if (viewBrushRadius <= 0.0f) {
        return;
    }

    const float strokeLeft   = juce::jmin(from.x, to.x) - viewBrushRadius - 1.0f;
    const float strokeRight  = juce::jmax(from.x, to.x) + viewBrushRadius + 1.0f;
    const float strokeTop    = juce::jmin(from.y, to.y) - viewBrushRadius - 1.0f;
    const float strokeBottom = juce::jmax(from.y, to.y) + viewBrushRadius + 1.0f;
    const float strokeX      = to.x - from.x;
    const float strokeY      = to.y - from.y;
    const float strokeSquare = strokeX * strokeX + strokeY * strokeY;

    const int lastCanvasColumn = canvasWidth - 1;
    const int lastCanvasRow    = canvasHeight - 1;
    const int leftCell         = static_cast<int>(std::floor(strokeLeft));
    const int rightCell        = static_cast<int>(std::ceil(strokeRight));
    const int topCell          = static_cast<int>(std::floor(strokeTop));
    const int bottomCell       = static_cast<int>(std::ceil(strokeBottom));
    const int firstColumn      = juce::jmax(0, leftCell);
    const int lastColumn       = juce::jmin(lastCanvasColumn, rightCell);
    const int firstRow         = juce::jmax(0, topCell);
    const int lastRow          = juce::jmin(lastCanvasRow, bottomCell);

    std::vector<float>& density = paintDensity[static_cast<size_t>(activePaintLayer)];

    if (lastColumn < firstColumn || lastRow < firstRow) {
        return;
    }

    for (int row = firstRow; row <= lastRow; ++row) {
        for (int column = firstColumn; column <= lastColumn; ++column) {
            const float offsetX = static_cast<float>(column) - from.x;
            const float offsetY = static_cast<float>(row) - from.y;
            float       along   = 0.0f;

            if (strokeSquare > 0.0f) {
                along = (offsetX * strokeX + offsetY * strokeY) / strokeSquare;
            }

            along = juce::jlimit(0.0f, 1.0f, along);

            const float  perpendicularX = offsetX - along * strokeX;
            const float  perpendicularY = offsetY - along * strokeY;
            const float  distance       = std::sqrt(perpendicularX * perpendicularX + perpendicularY * perpendicularY);
            const size_t index          = static_cast<size_t>(row) * static_cast<size_t>(canvasWidth) + static_cast<size_t>(column);

            if (distance >= viewBrushRadius) {
                continue;
            }

            const float falloff  = 1.0f - distance / viewBrushRadius;
            const float coverage = falloff * falloff;

            if (rearm) {
                strokeMask[index] *= (1.0f - dwellRearm);
            }

            if (coverage <= strokeMask[index]) {
                continue;
            }

            const float delta   = brushFlow * (coverage - strokeMask[index]);
            float       painted = density[index] + delta;

            strokeMask[index] = coverage;

            if (brushStroke == BrushStroke::Erasing) {
                painted = density[index] - delta;
            }

            density[index] = juce::jlimit(0.0f, 1.0f, painted);
        }
    }
}

void ValueField::applyPaintToNodes(juce::Point<float> from, juce::Point<float> to)
{
    const int canvasWidth  = nodeCanvas.getWidth();
    const int canvasHeight = nodeCanvas.getHeight();

    if (canvasWidth <= 0 || canvasHeight <= 0 || nodeCanvas.nodeManager.all().empty()) {
        return;
    }

    const juce::Identifier valueId      = paintLayerValueId();
    const float            strokeX      = to.x - from.x;
    const float            strokeY      = to.y - from.y;
    const float            strokeSquare = strokeX * strokeX + strokeY * strokeY;

    if (paintDensity[static_cast<size_t>(activePaintLayer)].size() != static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight)) {
        return;
    }

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const auto  nodeCentre = node->getNodeCentre().toFloat().transformedBy(nodeCanvas.viewTransform);
        const float offsetX    = nodeCentre.x - from.x;
        const float offsetY    = nodeCentre.y - from.y;
        const float reach      = (brushRadius + node->getVisualRadius()) * viewZoom;
        float       along      = 0.0f;

        if (strokeSquare > 0.0f) {
            along = (offsetX * strokeX + offsetY * strokeY) / strokeSquare;
        }

        along = juce::jlimit(0.0f, 1.0f, along);

        const float perpendicularX = offsetX - along * strokeX;
        const float perpendicularY = offsetY - along * strokeY;

        if (perpendicularX * perpendicularX + perpendicularY * perpendicularY > reach * reach) {
            continue;
        }

        const Node& graphNode = static_cast<const Node&>(*node);

        const std::optional<float> sampled = densityUnderNode(graphNode);

        if (!sampled.has_value()) {
            continue;
        }

        const float scaledValue = *sampled * maximumMidiValue;

        const int roundedValue = static_cast<int>(std::round(scaledValue));
        const int paintedValue = juce::jlimit(0, maximumDataValue, roundedValue);

        juce::ValueTree note = nodeCanvas.graphState.getMidiNotes(nodeId).getChild(0);

        if (!note.isValid()) {
            continue;
        }

        if (static_cast<int>(note.getProperty(valueId)) != paintedValue) {
            note.setProperty(valueId, paintedValue, nullptr);
        }
    }
}

void ValueField::ensurePaintBuffers()
{
    const size_t        pixelCount = static_cast<size_t>(nodeCanvas.getWidth()) * static_cast<size_t>(nodeCanvas.getHeight());
    std::vector<float>& density    = paintDensity[static_cast<size_t>(activePaintLayer)];

    if (density.size() != pixelCount) {
        density.assign(pixelCount, 0.0f);
    }

    if (strokeMask.size() != pixelCount) {
        strokeMask.assign(pixelCount, 0.0f);
    }
}

void ValueField::seedStrokeDensityFromNodes()
{
    const int canvasWidth      = nodeCanvas.getWidth();
    const int canvasHeight     = nodeCanvas.getHeight();
    const int lastCanvasColumn = canvasWidth - 1;
    const int lastCanvasRow    = canvasHeight - 1;

    if (canvasWidth <= 0 || canvasHeight <= 0) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();
    std::vector<float>&    density = paintDensity[static_cast<size_t>(activePaintLayer)];

    for (auto& [nodeId, node] : nodeCanvas.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const juce::ValueTree note = nodeCanvas.graphState.getMidiNotes(nodeId).getChild(0);

        if (!note.isValid()) {
            continue;
        }

        const int noteValue = note.getProperty(valueId);

        const auto nodeCentre = node->getNodeCentre().toFloat().transformedBy(nodeCanvas.viewTransform);

        const float seedDensity  = juce::jlimit(0.0f, 1.0f, noteValue / maximumMidiValue);
        const float nodeRadius   = node->getVisualRadius() * viewZoom;
        const float radiusSquare = nodeRadius * nodeRadius;

        const int leftCell    = static_cast<int>(std::floor(nodeCentre.x - nodeRadius));
        const int rightCell   = static_cast<int>(std::ceil (nodeCentre.x + nodeRadius));
        const int topCell     = static_cast<int>(std::floor(nodeCentre.y - nodeRadius));
        const int bottomCell  = static_cast<int>(std::ceil (nodeCentre.y + nodeRadius));
        const int firstColumn = juce::jmax(0, leftCell);
        const int lastColumn  = juce::jmin(lastCanvasColumn, rightCell);
        const int firstRow    = juce::jmax(0, topCell);
        const int lastRow     = juce::jmin(lastCanvasRow, bottomCell);

        for (int row = firstRow; row <= lastRow; ++row) {
            const float offsetY = static_cast<float>(row) - nodeCentre.y;

            for (int column = firstColumn; column <= lastColumn; ++column) {
                const float offsetX = static_cast<float>(column) - nodeCentre.x;

                if (offsetX * offsetX + offsetY * offsetY > radiusSquare) {
                    continue;
                }

                density[static_cast<size_t>(row) * static_cast<size_t>(canvasWidth) + static_cast<size_t>(column)] = seedDensity;
            }
        }
    }
}

void ValueField::timerCallback()
{
    if (brushStroke == BrushStroke::Idle) {
        stopTimer();
        return;
    }

    accumulateStroke(brushCurrentPoint, brushCurrentPoint, true);
    applyPaintToNodes(brushCurrentPoint, brushCurrentPoint);
}

juce::Identifier ValueField::paintLayerValueId() const
{
    switch (activePaintLayer) {
        case PaintLayer::Duration: return ValueTreeIdentifiers::MidiDuration;
        case PaintLayer::Velocity: return ValueTreeIdentifiers::MidiVelocity;
        case PaintLayer::Pitch:    return ValueTreeIdentifiers::MidiPitch;
    }

    return ValueTreeIdentifiers::MidiPitch;
}

juce::Colour ValueField::mapFieldColour(float factor) const
{
    const float boosted    = factor * colourBoost;
    const float overflow   = boosted - 1.0f;
    const float brightness = juce::jlimit(0.0f, 1.0f, boosted);
    const float whiteMix   = juce::jlimit(0.0f, maximumWhiteMix, overflow);

    return brushColour.withMultipliedBrightness(brightness).interpolatedWith(juce::Colours::white, whiteMix);
}

std::optional<float> ValueField::densityUnderNode(const Node& node) const
{
    const int canvasWidth      = nodeCanvas.getWidth();
    const int canvasHeight     = nodeCanvas.getHeight();
    const int lastCanvasColumn = canvasWidth - 1;
    const int lastCanvasRow    = canvasHeight - 1;

    const std::vector<float>& density = paintDensity[static_cast<size_t>(activePaintLayer)];

    const bool isErasing = brushStroke == BrushStroke::Erasing;

    const auto nodeCentre = node.getNodeCentre().toFloat().transformedBy(nodeCanvas.viewTransform);

    const float nodeRadius   = node.getVisualRadius() * viewZoom;
    const float radiusSquare = nodeRadius * nodeRadius;

    const int leftCell    = static_cast<int>(std::floor(nodeCentre.x - nodeRadius));
    const int rightCell   = static_cast<int>(std::ceil (nodeCentre.x + nodeRadius));
    const int topCell     = static_cast<int>(std::floor(nodeCentre.y - nodeRadius));
    const int bottomCell  = static_cast<int>(std::ceil (nodeCentre.y + nodeRadius));
    const int firstColumn = juce::jmax(0, leftCell);
    const int lastColumn  = juce::jmin(lastCanvasColumn, rightCell);
    const int firstRow    = juce::jmax(0, topCell);
    const int lastRow     = juce::jmin(lastCanvasRow, bottomCell);

    float sampled = 0.0f;

    bool foundCoverage = false;

    if (isErasing) {
        sampled = 1.0f;
    }

    for (int row = firstRow; row <= lastRow; ++row) {
        const float offsetY = static_cast<float>(row) - nodeCentre.y;

        for (int column = firstColumn; column <= lastColumn; ++column) {
            const float offsetX = static_cast<float>(column) - nodeCentre.x;

            if (offsetX * offsetX + offsetY * offsetY > radiusSquare) {
                continue;
            }

            const float pixelDensity = density[static_cast<size_t>(row) * static_cast<size_t>(canvasWidth) + static_cast<size_t>(column)];

            if (isErasing) {
                sampled = juce::jmin(sampled, pixelDensity);
            }
            else {
                sampled = juce::jmax(sampled, pixelDensity);
            }

            foundCoverage = true;
        }
    }

    if (!foundCoverage) {
        return std::nullopt;
    }

    return sampled;
}
