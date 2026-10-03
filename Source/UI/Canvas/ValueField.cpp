#include "ValueField.h"
#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/GraphState.h"
#include "../../Graph/ValueTreeIdentifiers.h"

ValueField::ValueField(NodeCanvas& owner) : owner(owner)
{
}

ValueField::~ValueField()
{
    stopTimer();
}

void ValueField::setActivePaintLayer(PaintLayer layer)
{
    activePaintLayer = layer;

    if (owner.paintMode) {
        render();
    }

    owner.repaint();
}

void ValueField::render()
{
    const int canvasWidth  = owner.getWidth();
    const int canvasHeight = owner.getHeight();

    if (canvasWidth <= 0 || canvasHeight <= 0) {
        return;
    }

    const int          fieldWidth  = juce::jmax(1, canvasWidth / fieldScale);
    const int          fieldHeight = juce::jmax(1, canvasHeight / fieldScale);
    const juce::Colour background  = CustomLookAndFeel::get(owner).canvasColour.brighter();

    accumulateNodeGlow(fieldWidth, fieldHeight);

    if (image.getWidth() != fieldWidth || image.getHeight() != fieldHeight) {
        image = juce::Image(juce::Image::ARGB, fieldWidth, fieldHeight, false);
    }

    juce::Image::BitmapData pixels(image, juce::Image::BitmapData::writeOnly);

    for (int row = 0; row < fieldHeight; ++row) {
        const size_t rowStart     = static_cast<size_t>(row) * fieldWidth;
        const float* weightedSum  = fieldWeightedSum.data()  + rowStart;
        const float* totalWeight  = fieldTotalWeight.data()  + rowStart;
        const float* coverageProd = fieldCoverageProd.data() + rowStart;
        juce::uint8* line         = pixels.getLinePointer(row);

        for (int column = 0; column < fieldWidth; ++column) {
            juce::Colour     pixelColour = background;
            juce::PixelARGB* pixel       = reinterpret_cast<juce::PixelARGB*>(line + column * pixels.pixelStride);

            if (totalWeight[column] > 0.0f) {
                const float fieldFactor = weightedSum[column] / totalWeight[column];
                const float coverage    = juce::jlimit(0.0f, 1.0f, 1.0f - coverageProd[column]);

                pixelColour = background.interpolatedWith(mapFieldColour(fieldFactor), coverage);
            }

            pixel->setARGB(pixelColour.getAlpha(), pixelColour.getRed(), pixelColour.getGreen(), pixelColour.getBlue());
        }
    }
}

void ValueField::accumulateNodeGlow(int fieldWidth, int fieldHeight)
{
    const juce::Identifier valueId           = paintLayerValueId();
    const float            fieldRadius       = glowRadius / static_cast<float>(fieldScale);
    const float            fieldRadiusSquare = fieldRadius * fieldRadius;
    const size_t           cellCount         = static_cast<size_t>(fieldWidth) * static_cast<size_t>(fieldHeight);

    fieldWeightedSum.assign(cellCount, 0.0f);
    fieldTotalWeight.assign(cellCount, 0.0f);
    fieldCoverageProd.assign(cellCount, 1.0f);

    for (auto& [nodeId, node] : owner.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const juce::ValueTree note = owner.applicationContext.graphState->getMidiNotes(nodeId).getChild(0);

        if (!note.isValid()) {
            continue;
        }

        const float valueFactor = juce::jlimit(0.0f, 1.0f, static_cast<int>(note.getProperty(valueId)) / maximumMidiValue);
        const auto  nodeCentre  = node->getBounds().getCentre().toFloat();
        const float centreX     = nodeCentre.x / static_cast<float>(fieldScale);
        const float centreY     = nodeCentre.y / static_cast<float>(fieldScale);
        const int   firstColumn = juce::jmax(0,               static_cast<int>(std::floor(centreX - fieldRadius)));
        const int   lastColumn  = juce::jmin(fieldWidth - 1,  static_cast<int>(std::ceil (centreX + fieldRadius)));
        const int   firstRow    = juce::jmax(0,               static_cast<int>(std::floor(centreY - fieldRadius)));
        const int   lastRow     = juce::jmin(fieldHeight - 1, static_cast<int>(std::ceil (centreY + fieldRadius)));

        for (int row = firstRow; row <= lastRow; ++row) {
            const float  offsetY      = static_cast<float>(row) - centreY;
            const size_t rowStart     = static_cast<size_t>(row) * fieldWidth;
            float*       weightedSum  = fieldWeightedSum.data()  + rowStart;
            float*       totalWeight  = fieldTotalWeight.data()  + rowStart;
            float*       coverageProd = fieldCoverageProd.data() + rowStart;

            for (int column = firstColumn; column <= lastColumn; ++column) {
                const float offsetX        = static_cast<float>(column) - centreX;
                const float distanceSquare = offsetX * offsetX + offsetY * offsetY;

                if (distanceSquare >= fieldRadiusSquare) {
                    continue;
                }

                const float weight = std::pow(1.0f - std::sqrt(distanceSquare) / fieldRadius, 10.0f);

                weightedSum[column]  += weight * valueFactor;
                totalWeight[column]  += weight;
                coverageProd[column] *= (1.0f - weight);
            }
        }
    }
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
    const float boosted    = factor * 1.6f;
    const float brightness = juce::jlimit(0.0f, 1.0f, boosted);
    const float whiteMix   = juce::jlimit(0.0f, 0.4f, boosted - 1.0f);

    return brushColour.withMultipliedBrightness(brightness).interpolatedWith(juce::Colours::white, whiteMix);
}

void ValueField::updateBrushCursor()
{
    if (!owner.paintMode) {
        return;
    }

    const int      cursorSize = juce::jmax(1, static_cast<int>(brushRadius * viewZoom * 2.0f));
    const int      hotspot    = cursorSize / 2;
    juce::Image    cursorImage(juce::Image::ARGB, cursorSize, cursorSize, true);
    juce::Graphics cursorGraphics(cursorImage);

    cursorGraphics.setColour(juce::Colours::black);
    cursorGraphics.drawEllipse(cursorImage.getBounds().toFloat().reduced(1.0f), 1.0f);

    owner.setMouseCursor(juce::MouseCursor(cursorImage, hotspot, hotspot));
}

void ValueField::refresh()
{
    if (!owner.paintMode) {
        return;
    }

    render();

    owner.repaint();
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

void ValueField::ensurePaintBuffers()
{
    const size_t        pixelCount = static_cast<size_t>(owner.getWidth()) * static_cast<size_t>(owner.getHeight());
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
    const int canvasWidth  = owner.getWidth();
    const int canvasHeight = owner.getHeight();

    if (canvasWidth <= 0 || canvasHeight <= 0) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();
    std::vector<float>&    density = paintDensity[static_cast<size_t>(activePaintLayer)];

    for (auto& [nodeId, node] : owner.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const juce::ValueTree note = owner.applicationContext.graphState->getMidiNotes(nodeId).getChild(0);

        if (!note.isValid()) {
            continue;
        }

        const float seedDensity  = juce::jlimit(0.0f, 1.0f, static_cast<int>(note.getProperty(valueId)) / maximumMidiValue);
        const auto  nodeCentre   = node->getNodeCentre().toFloat();
        const float nodeRadius   = node->getVisualRadius();
        const float radiusSquare = nodeRadius * nodeRadius;
        const int   firstColumn  = juce::jmax(0,                static_cast<int>(std::floor(nodeCentre.x - nodeRadius)));
        const int   lastColumn   = juce::jmin(canvasWidth - 1,  static_cast<int>(std::ceil (nodeCentre.x + nodeRadius)));
        const int   firstRow     = juce::jmax(0,                static_cast<int>(std::floor(nodeCentre.y - nodeRadius)));
        const int   lastRow      = juce::jmin(canvasHeight - 1, static_cast<int>(std::ceil (nodeCentre.y + nodeRadius)));

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

void ValueField::accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm)
{
    const int canvasWidth  = owner.getWidth();
    const int canvasHeight = owner.getHeight();

    if (canvasWidth <= 0 || canvasHeight <= 0) {
        return;
    }

    ensurePaintBuffers();

    if (brushRadius <= 0.0f) {
        return;
    }

    const int           firstColumn  = juce::jmax(0,                static_cast<int>(std::floor(juce::jmin(from.x, to.x) - brushRadius - 1.0f)));
    const int           lastColumn   = juce::jmin(canvasWidth - 1,  static_cast<int>(std::ceil (juce::jmax(from.x, to.x) + brushRadius + 1.0f)));
    const int           firstRow     = juce::jmax(0,                static_cast<int>(std::floor(juce::jmin(from.y, to.y) - brushRadius - 1.0f)));
    const int           lastRow      = juce::jmin(canvasHeight - 1, static_cast<int>(std::ceil (juce::jmax(from.y, to.y) + brushRadius + 1.0f)));
    const float         strokeX      = to.x - from.x;
    const float         strokeY      = to.y - from.y;
    const float         strokeSquare = strokeX * strokeX + strokeY * strokeY;
    std::vector<float>& density      = paintDensity[static_cast<size_t>(activePaintLayer)];

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

            if (distance >= brushRadius) {
                continue;
            }

            const float falloff  = 1.0f - distance / brushRadius;
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
    const int canvasWidth  = owner.getWidth();
    const int canvasHeight = owner.getHeight();

    if (canvasWidth <= 0 || canvasHeight <= 0 || owner.nodeManager.all().empty()) {
        return;
    }

    const juce::Identifier valueId      = paintLayerValueId();
    const float            strokeX      = to.x - from.x;
    const float            strokeY      = to.y - from.y;
    const float            strokeSquare = strokeX * strokeX + strokeY * strokeY;

    if (paintDensity[static_cast<size_t>(activePaintLayer)].size() != static_cast<size_t>(canvasWidth) * static_cast<size_t>(canvasHeight)) {
        return;
    }

    for (auto& [nodeId, node] : owner.nodeManager.all()) {
        if (node == nullptr) {
            continue;
        }

        const auto  nodeCentre = node->getNodeCentre().toFloat();
        const float offsetX    = nodeCentre.x - from.x;
        const float offsetY    = nodeCentre.y - from.y;
        const float reach      = brushRadius + node->getVisualRadius();
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

        const std::optional<float> sampled = densityUnderNode(*node);

        if (!sampled.has_value()) {
            continue;
        }

        const int       paintedValue = juce::jlimit(0, 127, static_cast<int>(std::round(*sampled * maximumMidiValue)));
        juce::ValueTree note         = owner.applicationContext.graphState->getMidiNotes(nodeId).getChild(0);

        if (!note.isValid()) {
            continue;
        }

        if (static_cast<int>(note.getProperty(valueId)) != paintedValue) {
            note.setProperty(valueId, paintedValue, nullptr);
        }
    }
}

std::optional<float> ValueField::densityUnderNode(const Node& node) const
{
    const int                 canvasWidth   = owner.getWidth();
    const int                 canvasHeight  = owner.getHeight();
    const std::vector<float>& density       = paintDensity[static_cast<size_t>(activePaintLayer)];
    const bool                isErasing     = brushStroke == BrushStroke::Erasing;
    const auto                nodeCentre    = node.getNodeCentre().toFloat();
    const float               nodeRadius    = node.getVisualRadius();
    const float               radiusSquare  = nodeRadius * nodeRadius;
    const int                 firstColumn   = juce::jmax(0,                static_cast<int>(std::floor(nodeCentre.x - nodeRadius)));
    const int                 lastColumn    = juce::jmin(canvasWidth - 1,  static_cast<int>(std::ceil (nodeCentre.x + nodeRadius)));
    const int                 firstRow      = juce::jmax(0,                static_cast<int>(std::floor(nodeCentre.y - nodeRadius)));
    const int                 lastRow       = juce::jmin(canvasHeight - 1, static_cast<int>(std::ceil (nodeCentre.y + nodeRadius)));
    float                     sampled       = 0.0f;
    bool                      foundCoverage = false;

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

void ValueField::endStroke()
{
    if (brushStroke == BrushStroke::Idle) {
        return;
    }

    brushStroke = BrushStroke::Idle;

    stopTimer();
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
