//
// Created by Eli Baumgardner on 6/29/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <array>
#include <vector>

class NodeCanvas;

class ValueField : public juce::Timer {

public:
    static constexpr int   numPaintLayers = 3;
    static constexpr int   dwellTimerHz   = 30;
    static constexpr float dwellRearm     = 0.15f;

    explicit ValueField(NodeCanvas& owner);
    ~ValueField() override;

    void setBrushColour(juce::Colour colour);
    void setBrushRadius(float radius);
    void setActivePaintLayer(int index);
    void setViewZoom(float z);
    void updateCursor();
    void refresh();
    void paintStroke(juce::Point<float> canvasPos, bool isStart, bool erase = false);
    void endStroke();

    juce::Image  image;
    juce::Colour brushColour      = juce::Colours::white;
    float        brushRadius      = 12.0f;
    float        brushFlow        = 0.22f;
    int          activePaintLayer = 0;

    std::array<std::vector<float>, numPaintLayers> paintDensity;

private:
    void render();
    void updateBrushCursor();
    void ensurePaintBuffers();
    void seedStrokeDensityFromNodes();
    void accumulateStroke(juce::Point<float> from, juce::Point<float> to, bool rearm = false);
    void applyPaintToNodes(juce::Point<float> from, juce::Point<float> to);
    juce::Colour     mapFieldColour(float factor) const;
    juce::Identifier paintLayerValueId() const;
    void timerCallback() override;

    NodeCanvas& owner;
    float viewZoom = 1.0f;

    std::vector<float> fieldWeightedSum;
    std::vector<float> fieldTotalWeight;
    std::vector<float> fieldCoverageProd;

    std::vector<float>  strokeMask;
    juce::Point<float>  strokePrevPoint;
    juce::Point<float>  brushCurrentPoint;
    bool brushStrokeActive = false;
    bool brushErase        = false;
};
