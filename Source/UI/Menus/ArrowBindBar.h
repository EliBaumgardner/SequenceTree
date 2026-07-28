//
// Created by Eli Baumgardner on 7/27/26.
//

#ifndef SEQUENCETREE_ARROWBINDBAR_H
#define SEQUENCETREE_ARROWBINDBAR_H

#include "../Bar.h"
#include "../Buttons/IconButton.h"
#include "../Node/ValueEditor.h"
#include "ItemSelector.h"

class ArrowBindBar : public Bar {

public:

    explicit ArrowBindBar(ApplicationContext& context);

    static constexpr int preferredHeight = 34;
    static constexpr int minimumHeight   = 26;

private:

    struct AxisControl {
        std::unique_ptr<IconButton>  toggle;
        std::unique_ptr<ValueEditor> editor;
    };

    struct BindField {
        AxisControl x;
        AxisControl y;
    };

    struct Metrics {
        int selectorWidth;
        int toggleWidth;
        int editorWidth;
        int axisGap;
        int itemGap;
    };

    void resized() override;

    void configureFieldSelector();
    void configureField(BindField& field);
    void configureAxis(AxisControl& axis, const juce::String& text);

    void showField(int itemId);

    Metrics metricsFor(juce::Rectangle<int> bounds) const;

    void layOutField(BindField& field, juce::Rectangle<int> bounds, const Metrics& metrics);
    void layOutAxis (AxisControl& axis, juce::Rectangle<int>& bounds, const Metrics& metrics);

    static int scaled(int total, float ratio, int minimum);

    static constexpr int pitchItemId    = 1;
    static constexpr int durationItemId = 2;

    static constexpr float selectorWidthRatio = 0.29f;
    static constexpr float toggleWidthRatio   = 0.10f;
    static constexpr float editorWidthRatio   = 0.14f;
    static constexpr float axisGapRatio       = 0.01f;
    static constexpr float itemGapRatio       = 0.03f;

    static constexpr float toggleFontRatio  = 0.4f;
    static constexpr float minimumFontHeight = 8.0f;

    static constexpr int minimumSelectorWidth = 34;
    static constexpr int minimumToggleWidth   = 14;
    static constexpr int minimumEditorWidth   = 18;
    static constexpr int minimumGap           = 1;

    ItemSelector fieldSelector;

    BindField pitchField;
    BindField durationField;
};

#endif //SEQUENCETREE_ARROWBINDBAR_H
