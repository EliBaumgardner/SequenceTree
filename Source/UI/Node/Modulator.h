//
// Created by Eli Baumgardner on 10/6/25.
//

#ifndef SEQUENCETREE_MODULATOR_H
#define SEQUENCETREE_MODULATOR_H

#include "../../Util/PluginModules.h"
#include "Node.h"

class NodeCanvas;

class Modulator : public Node {

    public:

    explicit Modulator(ApplicationContext& context);
    void  paint(juce::Graphics& g) override;
    bool  hitTest(int x, int y) override;
    void  setDisplayMode(NodeDisplayMode mode) override;

    juce::Rectangle<float> getSquareBounds() const;

    static constexpr int minimumPitchOffset = -48;
    static constexpr int maximumPitchOffset =  48;

    static constexpr float equalAreaSideFactor = 0.8862f;
};

#endif //SEQUENCETREE_MODULATOR_H
