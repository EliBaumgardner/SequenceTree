//
// Created by Eli Baumgardner on 7/27/26.
//

#ifndef SEQUENCETREE_ARROWBINDBAR_H
#define SEQUENCETREE_ARROWBINDBAR_H

#include "../../Util/ArrowInfo.h"
#include "../Bar.h"
#include "../Editors/ValueEditor.h"
#include "ItemSelector.h"

class ArrowBindBar : public Bar {

public:

    explicit ArrowBindBar(ApplicationContext& context);

    static constexpr int preferredHeight = 34;
    static constexpr int minimumHeight   = 26;

private:

    struct AxisControl {
        juce::Label                  label;
        std::unique_ptr<ValueEditor> editor;
    };

    struct BindField {
        AxisControl x;
        AxisControl y;
    };

    using AxisMember = AxisControl BindField::*;

    struct Metrics {
        int   selectorWidth;
        int   controlWidth;
        int   axisGap;
        int   itemGap;
        float fontHeight;
    };

    void resized() override;

    void configureFieldSelector();
    void configureField(BindField& field, BindField& otherField);
    void configureAxis (AxisControl& axis, AxisControl& otherAxis, const juce::String& text);

    void showField(int itemId);

    void showCurrentBindings();
    void showAxis(AxisMember axisMember, ArrowBinding binding, double multiplier);

    void publishBindings();
    void resolveAxis(AxisMember axisMember, ArrowBinding& binding, double& multiplier) const;

    Metrics metricsFor(juce::Rectangle<int> bounds) const;

    void layOutField(BindField& field, juce::Rectangle<int> bounds, const Metrics& metrics);
    void layOutAxis (AxisControl& axis, juce::Rectangle<int>& bounds, const Metrics& metrics);

    static int scaled(int total, float ratio, int minimum);

    static constexpr AxisMember xAxis = &BindField::x;
    static constexpr AxisMember yAxis = &BindField::y;

    static constexpr double deactivatedMultiplier = 0.0;
    static constexpr double defaultMultiplier     = 1.0;
    static constexpr double maximumMultiplier     = 100.0;

    static constexpr int pitchItemId    = 1;
    static constexpr int durationItemId = 2;

    static constexpr float selectorWidthRatio = 0.29f;
    static constexpr float controlWidthRatio  = 0.14f;
    static constexpr float axisGapRatio       = 0.01f;
    static constexpr float itemGapRatio       = 0.03f;

    static constexpr float fontHeightRatio   = 0.4f;
    static constexpr float minimumFontHeight = 8.0f;

    static constexpr int minimumSelectorWidth = 34;
    static constexpr int minimumControlWidth  = 18;
    static constexpr int minimumGap           = 1;

    ItemSelector fieldSelector;

    BindField pitchField;
    BindField durationField;
};

#endif //SEQUENCETREE_ARROWBINDBAR_H
