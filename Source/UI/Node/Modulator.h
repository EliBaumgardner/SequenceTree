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
    void  resized() override;
    bool  hitTest(int x, int y) override;
    float getVisualRadius() const override;
    void  setDisplayMode(NodeDisplayMode mode) override;

    juce::Rectangle<float> getSquareBounds() const;

    static constexpr int minimumPitchOffset = -48;
    static constexpr int maximumPitchOffset =  48;

    static constexpr float cornerEditorHeightFactor = 0.30f;
    static constexpr float cornerEditorWidthFactor  = 0.45f;
};

#endif //SEQUENCETREE_MODULATOR_H
