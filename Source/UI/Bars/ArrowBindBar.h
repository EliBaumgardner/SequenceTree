#pragma once

#include "../../Util/ArrowInfo.h"
#include "Bar.h"
#include "../Editors/LabeledEditor.h"
#include "../Menus/ItemSelector.h"

class ArrowBindBar : public Bar
{
public:

    explicit ArrowBindBar(const ApplicationContext& context);

    static constexpr int   preferredHeight   = 34;
    static constexpr int   minimumHeight     = 26;
    static constexpr float bindBarInsetRatio = 0.154f;

private:

    struct BindField
    {
        std::unique_ptr<LabeledEditor> x;
        std::unique_ptr<LabeledEditor> y;
    };

    using AxisMember = std::unique_ptr<LabeledEditor> BindField::*;

    struct Metrics
    {
        int   selectorWidth;
        int   controlWidth;
        int   axisGap;
        int   itemGap;
        float fontHeight;
    };

    void resized() override;

    void configureAxis(std::unique_ptr<LabeledEditor>& axis, std::unique_ptr<LabeledEditor>& otherAxis, const juce::String& text);
    void publishBindings();
    void resolveAxis(AxisMember axisMember, ArrowBinding& binding, double& multiplier) const;
    void showField(int itemId);
    void showAxis(AxisMember axisMember, ArrowBinding binding, double multiplier);
    void layOutAxis(LabeledEditor& axis, juce::Rectangle<int>& bounds, const Metrics& metrics);

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
