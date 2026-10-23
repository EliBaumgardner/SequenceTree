#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include "../Buttons/PaintToolSettings.h"
#include <ArsNovaUI/Buttons/ValueSlider.h>
#include <ArsNovaUI/Menus/ContextMenu.h>
#include <ArsNovaUI/PopupWindow.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Ornaments/OrnamentWindow.h"
#include "../../Util/ArrowInfo.h"
#include "../../Util/NodeInfo.h"

#include <array>

/*
    The bar along the bottom of the editor, holding the controls for the canvas tools: the paint
    settings, a note preview button with a counts field, a span tool button, an ornament tool
    button, and two buttons that bind an arrow's x and y extent to pitch or duration. It is a
    Bar drawn as frosted glass over the NodeCanvas.

    Each tool button toggles NodeCanvas::activeTool, and the bar sets which of them looks selected
    from NodeCanvas::onActiveToolChanged. Right-clicking the note preview button opens a menu to
    repeat the preview, which goes to the audio thread as a preview request on TraversalSession.
    Right-clicking the ornament button rescans the OrnamentLibraries and opens the OrnamentWindow;
    the button shows the active ornament's symbol and name, updated when the libraries tree
    changes. Each bind button turns its axis of ArrowManager::currentArrowInfo on or off, and its
    right-click menu picks pitch or duration and a multiplier from 0.25x to 4x. applyDisplayMode
    sets the paint layer when Titlebar changes the display mode.

    PluginEditor owns it and asks narrowestWidth for the smallest window the controls fit in.
*/

class NodeCanvas;
class TraversalSession;
class OrnamentLibraries;


class BottomBar : public ArsNovaUI::Bar, private juce::ValueTree::Listener
{
public:

    BottomBar(NodeCanvas& nodeCanvas, TraversalSession& traversalSession, juce::ValueTree colourPresets, juce::UndoManager& undoManager,
              OrnamentLibraries& ornamentLibraries, juce::UndoManager& ornamentUndoManager);

    ~BottomBar() override;

    void resized() override;

    void applyDisplayMode(NodeInfo::DisplayMode mode);

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
        ArsNovaUI::IconButton*                    button;
        ArrowInfo::Binding                        target;
        ArrowInfo::Binding ArrowInfo::Settings::* binding;
        double ArrowInfo::Settings::*             multiplier;
        ArsNovaUI::ValueSlider                    multiplierSlider;
    };

    struct BindTarget
    {
        const char*        label;
        ArrowInfo::Binding binding;
    };

    static constexpr std::array<BindTarget, 2> bindTargets {{
        { "pitch",    ArrowInfo::Binding::PitchBind },
        { "duration", ArrowInfo::Binding::DurationBind }
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


    PaintToolSettings        paintPanel;
    ArsNovaUI::ButtonPane    toolPane;
    ArsNovaUI::ButtonPane    quaverPane;
    ArsNovaUI::ButtonPane    bindPane;
    ArsNovaUI::ButtonPane    ornamentPane;
    ArsNovaUI::LabeledEditor countsField;

    ArsNovaUI::IconButton* quaverTool     = nullptr;
    ArsNovaUI::IconButton* spanTool       = nullptr;
    ArsNovaUI::IconButton* ornamentButton = nullptr;

    ArsNovaUI::PopupWindowLauncher ornamentLauncher {
        "Ornaments",
        [this]() {
            auto content = std::make_unique<OrnamentWindow>(ArsNovaUI::CustomLookAndFeel::get(*this), ornamentLibraries, ornamentUndoManager);

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        }
    };

    BindAxis xAxis { nullptr, ArrowInfo::Binding::DurationBind, &ArrowInfo::Settings::xBinding, &ArrowInfo::Settings::xMultiplier };
    BindAxis yAxis { nullptr, ArrowInfo::Binding::PitchBind,    &ArrowInfo::Settings::yBinding, &ArrowInfo::Settings::yMultiplier };
};
