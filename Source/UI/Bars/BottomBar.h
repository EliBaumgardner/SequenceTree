#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Editors/LabeledEditor.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Buttons/ValueSlider.h"
#include "../Menus/ContextMenu.h"
#include "../../Util/ArrowInfo.h"
#include "../../Util/NodeInfo.h"

#include <array>

class BottomBar : public Bar
{
public:

    explicit BottomBar(const ApplicationContext& context);

    void resized() override;

    void applyDisplayMode(NodeDisplayMode mode);

private:

    struct BindAxis
    {
        IconButton*              button;
        ArrowBinding             target;
        ArrowBinding ArrowInfo::* binding;
        double ArrowInfo::*       multiplier;
        ValueSlider              multiplierSlider;
    };

    struct BindTarget
    {
        const char*  label;
        ArrowBinding binding;
    };

    void configureAxis(BindAxis& axis, const juce::String& text);
    void toggleAxis(BindAxis& axis);
    void showAxisMenu(BindAxis& axis);
    void showQuaverMenu();

    static constexpr std::array<BindTarget, 2> bindTargets {{
        { "pitch",    ArrowBinding::PitchBind },
        { "duration", ArrowBinding::DurationBind }
    }};

    static constexpr float paintPanelWidthRatio    = 0.26f;
    static constexpr float quaverPaneWidthRatio    = 0.105f;
    static constexpr float quaverButtonWidthRatio  = 0.28f;
    static constexpr float countsLabelWidthRatio   = 0.42f;

    static constexpr double minimumBindMultiplier  = 0.25;
    static constexpr double maximumBindMultiplier  = 4.0;
    static constexpr double bindMultiplierInterval = 0.25;

    static constexpr int multiplierSliderWidth = 150;

    PaintToolSettings paintPanel   { applicationContext };
    ButtonPane        toolPane     { applicationContext };
    ButtonPane        quaverPane   { applicationContext };
    ButtonPane        bindPane     { applicationContext };
    LabeledEditor     countsField  { applicationContext };

    IconButton* quaverTool = nullptr;
    IconButton* spanTool   = nullptr;

    BindAxis xAxis { nullptr, ArrowBinding::DurationBind, &ArrowInfo::xBinding, &ArrowInfo::xMultiplier };
    BindAxis yAxis { nullptr, ArrowBinding::PitchBind,    &ArrowInfo::yBinding, &ArrowInfo::yMultiplier };
};
