#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <optional>
#include <vector>

class NodeCanvas;
class Node;

class ValueField : public juce::Timer
{
public:

    enum class PaintLayer { Pitch, Duration, Velocity };

    enum class BrushStroke { Idle, Painting, Erasing };

    static constexpr int   paintLayerCount  = 3;
    static constexpr int   dwellTimerHz     = 30;
    static constexpr float dwellRearm       = 0.15f;
    static constexpr int   fieldScale       = 2;
    static constexpr float glowRadius       = 330.0f;
    static constexpr float maximumMidiValue = 127.0f;

    explicit ValueField(NodeCanvas& owner);
    ~ValueField() override;

    void setActivePaintLayer(PaintLayer layer);
    void updateBrushCursor();
    void refresh();
    void paintStroke(juce::Point<float> canvasPosition, bool isStart, bool erase = false);
    void endStroke();

    juce::Image  image;
    juce::Colour brushColour      = juce::Colours::white;
    float        brushRadius      = 12.0f;
    float        brushFlow        = 0.22f;
    PaintLayer   activePaintLayer = PaintLayer::Pitch;
    float        viewZoom         = 1.0f;

    std::array<std::vector<float>, paintLayerCount> paintDensity;

private:

    void render();
    void accumulateNodeGlow(int fieldWidth, int fieldHeight);
    void ensurePaintBuffers();
    void seedStrokeDensityFromNodes();
    void accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm = false);
    void applyPaintToNodes(juce::Point<float> from, juce::Point<float> to);
    std::optional<float> densityUnderNode(const Node& node) const;
    juce::Colour     mapFieldColour(float factor) const;
    juce::Identifier paintLayerValueId() const;

    void timerCallback() override;

    NodeCanvas& owner;

    std::vector<float> fieldWeightedSum;
    std::vector<float> fieldTotalWeight;
    std::vector<float> fieldCoverageProd;

    std::vector<float> strokeMask;
    juce::Point<float> strokePreviousPoint;
    juce::Point<float> brushCurrentPoint;
    BrushStroke        brushStroke = BrushStroke::Idle;
};
