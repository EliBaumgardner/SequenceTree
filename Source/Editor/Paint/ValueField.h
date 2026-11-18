#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <optional>
#include <vector>

/*
    The paint mode of the canvas: a coloured glow over the canvas that shows one value of every
    node (pitch, duration or velocity, the PaintLayer) as brightness around the node, and a brush
    that changes those values by painting. For each of the three layers it keeps paintDensity, a
    number from 0 to 1 for every pixel of the canvas, which the brush raises or, when erasing,
    lowers.

    paintStroke applies the brush between two points: it adds to the density under the brush and
    sets the value of each node the stroke touches to the highest (or, erasing, lowest) density
    under that node, scaled to 0..127, written to the node's first MIDI note without an undo
    transaction. endStroke finishes the stroke. While a stroke is held still, a timer keeps
    painting at the same point. refresh redraws the whole glow into image, which covers the graph
    rather than the screen, and refreshAround redraws only the part one node's glow reaches.
    NodeCanvas::paint draws image through imageToGraph and the view transform while the Paint
    tool is active, so panning and zooming redraw nothing, and setActivePaintLayer switches layers.

    NodeController calls paintStroke and endStroke from mouse events, PaintToolSettings sets the
    brush colour, radius and flow, and NodeCanvas owns it and refreshes it after changes to the
    graph and the view.
*/

class NodeCanvas;
class Node;

class ValueField : public juce::Timer
{
public:

    enum class PaintLayer;

    explicit ValueField(NodeCanvas& nodeCanvas);
    ~ValueField() override;

    void setActivePaintLayer(PaintLayer layer);
    void updateBrushCursor ();

    void refreshAround(int nodeId);
    void refresh ();

    void paintStroke(juce::Point<float> canvasPosition, bool isStart, bool erase = false);
    void endStroke  ();

    enum class PaintLayer { Pitch, Duration, Velocity };

    enum class BrushStroke { Idle, Painting, Erasing };

    static constexpr int paintLayerCount  = 3;
    static constexpr int dwellTimerHz     = 30;
    static constexpr int maximumFieldSide = 1024;
    static constexpr int falloffSteps     = 4096;
    static constexpr int colourSteps      = 1024;
    static constexpr int maximumDataValue = 127;

    static constexpr float dwellRearm         = 0.15f;
    static constexpr float fieldUnitsPerPixel = 4.0f;
    static constexpr float glowRadius         = 330.0f;
    static constexpr float maximumMidiValue   = 127.0f;
    static constexpr float falloffExponent    = 10.0f;
    static constexpr float colourBoost        = 1.6f;
    static constexpr float maximumWhiteMix    = 0.4f;

    juce::Image image;

    juce::AffineTransform imageToGraph;

    juce::Colour brushColour = juce::Colours::white;
    float        brushRadius = 12.0f;
    float        brushFlow   = 0.22f;

    PaintLayer activePaintLayer = PaintLayer::Pitch;
    float      viewZoom         = 1.0f;

    std::array<std::vector<float>, paintLayerCount> paintDensity;

private:

    void render ();
    void renderArea        (juce::Rectangle<int> area);
    void accumulateNodeGlow(juce::Rectangle<int> area);

    void accumulateStroke          (juce::Point<float> from, juce::Point<float> to, bool rearm = false);
    void applyPaintToNodes         (juce::Point<float> from, juce::Point<float> to);
    void ensurePaintBuffers        ();
    void seedStrokeDensityFromNodes();

    void timerCallback() override;

    juce::Identifier paintLayerValueId() const;
    juce::Colour     mapFieldColour   (float factor) const;

    std::optional<float> densityUnderNode(const Node& node) const;

    struct Stroke
    {
        std::vector<float> mask;

        juce::Point<float> previousPoint;
        juce::Point<float> currentPoint;

        BrushStroke state = BrushStroke::Idle;
    };

    NodeCanvas& nodeCanvas;

    std::vector<float> fieldWeightedSum;
    std::vector<float> fieldTotalWeight;
    std::vector<float> fieldCoverageProd;

    Stroke stroke;

    std::array<float, falloffSteps + 1>       falloffTable {};
    std::array<juce::Colour, colourSteps + 1> colourTable;

    juce::AffineTransform graphToImage;

    float fieldCellSize = fieldUnitsPerPixel;
};
