#pragma once

#include "Bar.h"
#include "../Buttons/IconButton.h"
#include "../Buttons/ButtonPane.h"
#include "../Editors/LabeledEditor.h"
#include "../Buttons/PaintToolSettings.h"
#include "../Buttons/ValueSlider.h"
#include "../Menus/ContextMenu.h"
#include "../PopupWindow.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Ornaments/OrnamentWindow.h"
#include "../../Util/ArrowInfo.h"
#include "../../Util/NodeInfo.h"

#include <array>

class NodeCanvas;
class TraversalSession;
class OrnamentLibraries;
class OrnamentApplier;

class BottomBar : public Bar, private juce::ValueTree::Listener
{
public:

    BottomBar(NodeCanvas& nodeCanvas, TraversalSession& traversalSession, juce::ValueTree colourPresets, juce::UndoManager& undoManager,
              OrnamentLibraries& ornamentLibraries, juce::UndoManager& ornamentUndoManager, OrnamentApplier& ornamentApplier);

    ~BottomBar() override;

    void resized() override;

    void applyDisplayMode(NodeDisplayMode mode);

    int narrowestWidth() const;

private:

    struct BindAxis;

    void configureOrnamentButton();
    void showActiveOrnamentName ();

    void configureAxis(BindAxis& axis, const juce::String& text);
    void toggleAxis   (BindAxis& axis);
    void showAxisMenu (BindAxis& axis);

    void showQuaverMenu();

    void valueTreePropertyChanged(juce::ValueTree& tree, const juce::Identifier& property) override;

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

    static constexpr std::array<BindTarget, 2> bindTargets {{
        { "pitch",    ArrowBinding::PitchBind },
        { "duration", ArrowBinding::DurationBind }
    }};

    static constexpr float quaverButtonWidthRatio = 0.28f;
    static constexpr float countsLabelWidthRatio  = 0.42f;

    static constexpr double minimumBindMultiplier  = 0.25;
    static constexpr double maximumBindMultiplier  = 4.0;
    static constexpr double bindMultiplierInterval = 0.25;

    static constexpr int multiplierSliderWidth = 150;
    static constexpr int paintPanelWidth       = 165;
    static constexpr int quaverPaneWidth       = 67;

    NodeCanvas&        nodeCanvas;
    TraversalSession&  traversalSession;
    OrnamentLibraries& ornamentLibraries;
    juce::UndoManager& ornamentUndoManager;
    OrnamentApplier&   ornamentApplier;

    PaintToolSettings paintPanel;
    ButtonPane        toolPane;
    ButtonPane        quaverPane;
    ButtonPane        bindPane;
    ButtonPane        ornamentPane;
    LabeledEditor     countsField;

    IconButton* quaverTool     = nullptr;
    IconButton* spanTool       = nullptr;
    IconButton* ornamentButton = nullptr;

    PopupWindowLauncher ornamentLauncher {
        "Ornaments",
        [this]() {
            auto content = std::make_unique<OrnamentWindow>(CustomLookAndFeel::get(*this), ornamentLibraries, ornamentUndoManager);

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        }
    };

    BindAxis xAxis { nullptr, ArrowBinding::DurationBind, &ArrowInfo::xBinding, &ArrowInfo::xMultiplier };
    BindAxis yAxis { nullptr, ArrowBinding::PitchBind,    &ArrowInfo::yBinding, &ArrowInfo::yMultiplier };
};
