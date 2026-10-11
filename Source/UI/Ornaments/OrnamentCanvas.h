#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "OrnamentNode.h"
#include "../Node/Arrow.h"

#include <map>
#include <memory>

class OrnamentCanvas : public juce::Component, public juce::AsyncUpdater
{
public:

    enum class ArrowPart { Shaft, Head, Label, DanglingTip };

    explicit OrnamentCanvas(juce::UndoManager& undoManager);

    void paint(juce::Graphics& graphics) override;

    void          childrenChanged  () override;
    void          handleAsyncUpdate() override;

    OrnamentNode* findNodeAt       (juce::Point<int> point) const;
    Arrow*        findArrowNear    (juce::Point<float> point, float radius, ArrowPart part) const;

    static constexpr int initialViewColumns = 4;
    static constexpr int tipGrabRadius      = 14;
    static constexpr int snapThreshold      = 5;

    juce::ValueTree       figure;
    juce::AffineTransform viewTransform;
    juce::AffineTransform modelTransform;

    std::map<int, std::unique_ptr<OrnamentNode>> nodes;
    juce::OwnedArray<Arrow>                      arrows;

private:

    void updateArrows();

    juce::UndoManager& undoManager;
};