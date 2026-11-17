#pragma once

#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ValueSlider.h>
#include "../../Util/ArrowInfo.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <array>

class BindPane : public ArsNovaUI::ButtonPane, private juce::ValueTree::Listener
{
public:

    explicit BindPane(juce::ValueTree canvasSettings);

    ~BindPane() override;

private:

    struct BindAxis;

    void configureAxis(BindAxis& axis, const juce::String& text);
    void toggleAxis   (BindAxis& axis);
    void showAxisMenu (BindAxis& axis);

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

    struct BindAxis
    {
        ArsNovaUI::IconButton* button;

        ArsNovaUI::ValueSlider multiplierSlider;

        juce::Identifier bindingProperty;
        juce::Identifier multiplierProperty;
        juce::Identifier targetProperty;
    };

    struct BindTarget
    {
        const char* label;

        ArrowInfo::Binding binding;
    };

    static constexpr std::array<BindTarget, 2> bindTargets {{
        { "pitch",    ArrowInfo::Binding::PitchBind },
        { "duration", ArrowInfo::Binding::DurationBind }
    }};

    static constexpr double minimumBindMultiplier  = 0.25;
    static constexpr double maximumBindMultiplier  = 4.0;
    static constexpr double bindMultiplierInterval = 0.25;

    static constexpr int multiplierSliderWidth = 150;

    juce::ValueTree canvasSettings;

    BindAxis xAxis { nullptr, {}, ValueTreeIdentifiers::ArrowXBinding, ValueTreeIdentifiers::ArrowXMultiplier, ValueTreeIdentifiers::ArrowXTarget };
    BindAxis yAxis { nullptr, {}, ValueTreeIdentifiers::ArrowYBinding, ValueTreeIdentifiers::ArrowYMultiplier, ValueTreeIdentifiers::ArrowYTarget };
};
