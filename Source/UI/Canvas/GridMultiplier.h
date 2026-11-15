#pragma once

#include <juce_data_structures/juce_data_structures.h>
#include <ArsNovaUI/Canvas/EditCanvas.h>

class GridMultiplier : public juce::ValueTree::Listener
{
public:

    GridMultiplier(ArsNovaUI::EditCanvas& canvas, juce::ValueTree canvasSettings, const juce::Identifier& multiplierProperty);
    ~GridMultiplier() override;

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    static constexpr double minimumMultiplier         = 1.0;
    static constexpr double maximumMultiplier         = 4.0;
    static constexpr double defaultGraphMultiplier    = 2.0;
    static constexpr double defaultOrnamentMultiplier = 1.0;

private:

    ArsNovaUI::EditCanvas& canvas;

    juce::ValueTree canvasSettings;

    juce::Identifier multiplierProperty;
};
