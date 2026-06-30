//
// Created by Eli Baumgardner on 6/29/26.
//

#include "ValueField.h"
#include "NodeCanvas.h"
#include "../Node/Node.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeState.h"
#include "../../Graph/ValueTreeIdentifiers.h"

ValueField::ValueField(NodeCanvas& ownerIn) : owner(ownerIn) {}

ValueField::~ValueField()
{
    stopTimer();
}

void ValueField::setBrushColour(juce::Colour colour)
{
    brushColour = colour;
}

void ValueField::setBrushRadius(float radius)
{
    brushRadius = radius;
    updateBrushCursor();
}

void ValueField::setActivePaintLayer(int index)
{
    activePaintLayer = juce::jlimit(0, numPaintLayers - 1, index);

    if (owner.paintMode) {
        render();
    }

    owner.repaint();
}

void ValueField::setViewZoom(float z)
{
    viewZoom = z;
    updateBrushCursor();
}

void ValueField::updateCursor()
{
    updateBrushCursor();
}

void ValueField::refresh()
{
    if (!owner.paintMode) {
        return;
    }

    render();
    owner.repaint();
}

void ValueField::paintStroke(juce::Point<float> canvasPos, bool isStart, bool erase)
{
    if (isStart) {
        brushStrokeActive = true;
        brushErase = erase;
        ensurePaintBuffers();
        std::fill(strokeMask.begin(), strokeMask.end(), 0.0f);
        seedStrokeDensityFromNodes();
        strokePrevPoint = canvasPos;
        startTimerHz(dwellTimerHz);
    }

    brushCurrentPoint = canvasPos;
    accumulateStroke(strokePrevPoint, canvasPos);
    applyPaintToNodes(strokePrevPoint, canvasPos);
    strokePrevPoint = canvasPos;
}

void ValueField::endStroke()
{
    if (!brushStrokeActive) {
        return;
    }

    brushStrokeActive = false;
    stopTimer();
}

void ValueField::timerCallback()
{
    if (!brushStrokeActive) {
        stopTimer();
        return;
    }

    accumulateStroke(brushCurrentPoint, brushCurrentPoint, true);
    applyPaintToNodes(brushCurrentPoint, brushCurrentPoint);
}

void ValueField::render()
{
    const int w = owner.getWidth();
    const int h = owner.getHeight();

    if (w <= 0 || h <= 0) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();

    static constexpr int   kFieldScale  = 2;
    static constexpr float glowRadius   = 330.0f;

    const int fieldW = juce::jmax(1, w / kFieldScale);
    const int fieldH = juce::jmax(1, h / kFieldScale);

    const float fieldRadius  = glowRadius / (float) kFieldScale;
    const float fieldRadius2 = fieldRadius * fieldRadius;

    const size_t numCells = (size_t) fieldW * (size_t) fieldH;
    fieldWeightedSum.assign(numCells, 0.0f);
    fieldTotalWeight.assign(numCells, 0.0f);
    fieldCoverageProd.assign(numCells, 1.0f);
    float* weightedSum  = fieldWeightedSum.data();
    float* totalWeight  = fieldTotalWeight.data();
    float* coverageProd = fieldCoverageProd.data();

    for (auto& [id, node] : owner.nodeMap) {
        if (node == nullptr) {
            continue;
        }

        juce::ValueTree notes = ValueTreeState::getMidiNotes(id);
        juce::ValueTree note  = notes.getChild(0);

        if (!note.isValid()) {
            continue;
        }

        const int   value  = (int) note.getProperty(valueId);
        const float factor = juce::jlimit(0.0f, 1.0f, value / 127.0f);

        const auto  centre = node->getBounds().getCentre().toFloat();
        const float cx = centre.x / (float) kFieldScale;
        const float cy = centre.y / (float) kFieldScale;

        const int x0 = juce::jmax(0,          (int) std::floor(cx - fieldRadius));
        const int x1 = juce::jmin(fieldW - 1, (int) std::ceil (cx + fieldRadius));
        const int y0 = juce::jmax(0,          (int) std::floor(cy - fieldRadius));
        const int y1 = juce::jmin(fieldH - 1, (int) std::ceil (cy + fieldRadius));

        for (int y = y0; y <= y1; ++y) {
            const float dy  = (float) y - cy;
            const float dy2 = dy * dy;
            float* wsRow = weightedSum  + (size_t) y * fieldW;
            float* twRow = totalWeight  + (size_t) y * fieldW;
            float* cpRow = coverageProd + (size_t) y * fieldW;

            for (int x = x0; x <= x1; ++x) {
                const float dx = (float) x - cx;
                const float d2 = dx * dx + dy2;

                if (d2 >= fieldRadius2) {
                    continue;
                }

                const float t = 1.0f - std::sqrt(d2) / fieldRadius;
                const float wt = std::pow(t, 10.0f);

                wsRow[x] += wt * factor;
                twRow[x] += wt;
                cpRow[x] *= (1.0f - wt);
            }
        }
    }

    if (image.getWidth() != fieldW || image.getHeight() != fieldH) {
        image = juce::Image(juce::Image::ARGB, fieldW, fieldH, false);
    }

    juce::Image::BitmapData pixels(image, juce::Image::BitmapData::writeOnly);
    const juce::Colour bg = CustomLookAndFeel::get(owner).canvasColour.brighter();

    for (int y = 0; y < fieldH; ++y) {
        const float*  wsRow = weightedSum  + (size_t) y * fieldW;
        const float*  twRow = totalWeight  + (size_t) y * fieldW;
        const float*  cpRow = coverageProd + (size_t) y * fieldW;
        juce::uint8*  line  = pixels.getLinePointer(y);

        for (int x = 0; x < fieldW; ++x) {
            juce::Colour out = bg;

            const float tw = twRow[x];
            if (tw > 0.0f) {
                const float fieldFactor = wsRow[x] / tw;
                const float coverage    = juce::jlimit(0.0f, 1.0f, 1.0f - cpRow[x]);
                out = bg.interpolatedWith(mapFieldColour(fieldFactor), coverage);
            }

            auto* px = (juce::PixelARGB*) (line + x * pixels.pixelStride);
            px->setARGB(out.getAlpha(), out.getRed(), out.getGreen(), out.getBlue());
        }
    }
}

void ValueField::updateBrushCursor()
{
    if (!owner.paintMode) {
        return;
    }

    const int size    = juce::jmax(1, (int)(brushRadius * viewZoom * 2.0f));
    const int hotspot = size / 2;

    juce::Image img(juce::Image::ARGB, size, size, true);
    juce::Graphics g(img);
    g.setColour(juce::Colours::black);
    g.drawEllipse(img.getBounds().toFloat().reduced(1.0f), 1.0f);

    owner.setMouseCursor(juce::MouseCursor(img, hotspot, hotspot));
}

void ValueField::ensurePaintBuffers()
{
    const size_t expected = (size_t) owner.getWidth() * (size_t) owner.getHeight();

    if (paintDensity[activePaintLayer].size() != expected) {
        paintDensity[activePaintLayer].assign(expected, 0.0f);
    }

    if (strokeMask.size() != expected) {
        strokeMask.assign(expected, 0.0f);
    }
}

void ValueField::seedStrokeDensityFromNodes()
{
    const int w = owner.getWidth();
    const int h = owner.getHeight();

    if (w <= 0 || h <= 0) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();
    std::vector<float>& density = paintDensity[activePaintLayer];

    for (auto& [id, node] : owner.nodeMap) {
        if (node == nullptr) {
            continue;
        }

        juce::ValueTree notes = ValueTreeState::getMidiNotes(id);
        juce::ValueTree note  = notes.getChild(0);

        if (!note.isValid()) {
            continue;
        }

        const int   value = (int) note.getProperty(valueId);
        const float seed  = juce::jlimit(0.0f, 1.0f, value / 127.0f);

        const auto  centre = node->getNodeCentre().toFloat();
        const float nodeR  = node->getBounds().getHeight() * 0.5f;
        const float nodeR2 = nodeR * nodeR;

        const int x0 = juce::jmax(0,     (int) std::floor(centre.x - nodeR));
        const int x1 = juce::jmin(w - 1, (int) std::ceil (centre.x + nodeR));
        const int y0 = juce::jmax(0,     (int) std::floor(centre.y - nodeR));
        const int y1 = juce::jmin(h - 1, (int) std::ceil (centre.y + nodeR));

        for (int y = y0; y <= y1; ++y) {
            const float ddy = (float) y - centre.y;
            for (int x = x0; x <= x1; ++x) {
                const float ddx = (float) x - centre.x;
                if (ddx * ddx + ddy * ddy > nodeR2) {
                    continue;
                }
                density[(size_t) y * (size_t) w + (size_t) x] = seed;
            }
        }
    }
}

void ValueField::accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm)
{
    const int w = owner.getWidth();
    const int h = owner.getHeight();

    if (w <= 0 || h <= 0) {
        return;
    }

    ensurePaintBuffers();

    const float r = brushRadius;
    if (r <= 0.0f) {
        return;
    }

    const int x0 = juce::jmax(0,     (int) std::floor(juce::jmin(from.x, to.x) - r - 1.0f));
    const int x1 = juce::jmin(w - 1, (int) std::ceil (juce::jmax(from.x, to.x) + r + 1.0f));
    const int y0 = juce::jmax(0,     (int) std::floor(juce::jmin(from.y, to.y) - r - 1.0f));
    const int y1 = juce::jmin(h - 1, (int) std::ceil (juce::jmax(from.y, to.y) + r + 1.0f));

    if (x1 < x0 || y1 < y0) {
        return;
    }

    const float dx      = to.x - from.x;
    const float dy      = to.y - from.y;
    const float segLen2 = dx * dx + dy * dy;

    std::vector<float>& density = paintDensity[activePaintLayer];

    for (int y = y0; y <= y1; ++y) {
        for (int x = x0; x <= x1; ++x) {
            const float px = (float) x - from.x;
            const float py = (float) y - from.y;

            float t = segLen2 > 0.0f ? (px * dx + py * dy) / segLen2 : 0.0f;
            t = juce::jlimit(0.0f, 1.0f, t);

            const float ox   = px - t * dx;
            const float oy   = py - t * dy;
            const float dist = std::sqrt(ox * ox + oy * oy);

            if (dist >= r) {
                continue;
            }

            const float falloff  = 1.0f - dist / r;
            const float coverage = falloff * falloff;

            const size_t index = (size_t) y * (size_t) w + (size_t) x;

            if (rearm) {
                strokeMask[index] *= (1.0f - dwellRearm);
            }

            if (coverage <= strokeMask[index]) {
                continue;
            }

            const float delta = brushFlow * (coverage - strokeMask[index]);
            strokeMask[index] = coverage;

            density[index] = juce::jlimit(0.0f, 1.0f,
                                          brushErase ? density[index] - delta
                                                     : density[index] + delta);
        }
    }
}

void ValueField::applyPaintToNodes(juce::Point<float> from, juce::Point<float> to)
{
    const int w = owner.getWidth();
    const int h = owner.getHeight();

    if (w <= 0 || h <= 0 || owner.nodeMap.empty()) {
        return;
    }

    const std::vector<float>& density = paintDensity[activePaintLayer];
    if (density.size() != (size_t) w * (size_t) h) {
        return;
    }

    const juce::Identifier valueId = paintLayerValueId();

    const float r       = brushRadius;
    const float dx      = to.x - from.x;
    const float dy      = to.y - from.y;
    const float segLen2 = dx * dx + dy * dy;

    for (auto& [id, node] : owner.nodeMap) {
        if (node == nullptr) {
            continue;
        }

        const auto  centre = node->getNodeCentre().toFloat();
        const float nodeR  = node->getBounds().getHeight() * 0.5f;

        const float px = centre.x - from.x;
        const float py = centre.y - from.y;

        float t = segLen2 > 0.0f ? (px * dx + py * dy) / segLen2 : 0.0f;
        t = juce::jlimit(0.0f, 1.0f, t);

        const float ox = px - t * dx;
        const float oy = py - t * dy;

        const float reach = r + nodeR;
        if (ox * ox + oy * oy > reach * reach) {
            continue;
        }

        const float nodeR2 = nodeR * nodeR;

        const int x0 = juce::jmax(0,     (int) std::floor(centre.x - nodeR));
        const int x1 = juce::jmin(w - 1, (int) std::ceil (centre.x + nodeR));
        const int y0 = juce::jmax(0,     (int) std::floor(centre.y - nodeR));
        const int y1 = juce::jmin(h - 1, (int) std::ceil (centre.y + nodeR));

        float sample = brushErase ? 1.0f : 0.0f;
        bool  found  = false;

        for (int y = y0; y <= y1; ++y) {
            const float ddy = (float) y - centre.y;
            for (int x = x0; x <= x1; ++x) {
                const float ddx = (float) x - centre.x;
                if (ddx * ddx + ddy * ddy > nodeR2) {
                    continue;
                }
                const float dv = density[(size_t) y * (size_t) w + (size_t) x];
                sample = brushErase ? juce::jmin(sample, dv) : juce::jmax(sample, dv);
                found  = true;
            }
        }

        if (!found) {
            continue;
        }

        const int value = juce::jlimit(0, 127, (int) std::round(sample * 127.0f));

        juce::ValueTree notes = ValueTreeState::getMidiNotes(id);
        juce::ValueTree note  = notes.getChild(0);

        if (!note.isValid()) {
            continue;
        }

        if ((int) note.getProperty(valueId) != value) {
            note.setProperty(valueId, value, nullptr);
        }
    }
}

juce::Colour ValueField::mapFieldColour(float factor) const
{
    const float boosted    = factor * 1.6f;
    const float brightness = juce::jlimit(0.0f, 1.0f, boosted);
    const float whiteMix   = juce::jlimit(0.0f, 0.4f, boosted - 1.0f);
    return brushColour.withMultipliedBrightness(brightness).interpolatedWith(juce::Colours::white, whiteMix);
}

juce::Identifier ValueField::paintLayerValueId() const
{
    if (activePaintLayer == 1) {
        return ValueTreeIdentifiers::MidiDuration;
    }

    if (activePaintLayer == 2) {
        return ValueTreeIdentifiers::MidiVelocity;
    }

    return ValueTreeIdentifiers::MidiPitch;
}
