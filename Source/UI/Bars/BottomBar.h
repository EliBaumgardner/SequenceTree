#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include "../Buttons/PaintToolSettings.h"
#include <ArsNovaUI/Buttons/ValueSlider.h>
#include <ArsNovaUI/Menus/ContextMenu.h>

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include "../../Util/ArrowInfo.h"
#include "../../Util/NodeInfo.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <array>

/*
    The bar along the bottom of the editor, holding the controls for the canvas tools: the paint
    settings, a note preview button with a counts field, a span tool button, an ornament tool
    button, and two buttons that bind an arrow's x and y extent to pitch or duration. It is a
    Bar drawn as frosted glass over the NodeCanvas.

    Each tool button toggles NodeCanvas::activeTool, and the bar sets which of them looks selected
    from NodeCanvas::onActiveToolChanged. Right-clicking the note preview button opens a menu to
    repeat the preview, which NotePreviewGestures::toggleRepeat sends to the audio thread.
    Right-clicking the ornament button calls onOrnamentWindowRequested, which PluginEditor answers
    by rescanning the OrnamentLibraries and opening the OrnamentWindow;
    the button shows the active ornament's symbol and name, updated when the libraries tree
    changes. Each bind button turns one axis of the arrow settings in canvasSettings, which
    GraphDocument reads for every new arrow, on or off, and its right-click menu picks pitch or
    duration and a multiplier from 0.25x to 4x. applyDisplayMode
    sets the paint layer when Titlebar changes the display mode.

    PluginEditor owns it and asks narrowestWidth for the smallest window the controls fit in.
*/

class NodeCanvas;
class GraphDocument;

class NotePreviewGestures;
class OrnamentLibraries;


class BottomBar : public ArsNovaUI::Bar, private juce::ValueTree::Listener
{
public:

    BottomBar(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, juce::ValueTree colourPresets, OrnamentLibraries& ornamentLibraries);

    ~BottomBar() override;

    void resized() override;

    void applyDisplayMode(NodeInfo::DisplayMode mode);

    int narrowestWidth() const;

    std::function<void()> onOrnamentWindowRequested;

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
        ArsNovaUI::IconButton* button;

        ArsNovaUI::ValueSlider multiplierSlider;

        juce::Identifier bindingProperty;
        juce::Identifier multiplierProperty;
        juce::Identifier targetProperty;
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

    static constexpr float captionHeightRatio         = 0.73f;
    static constexpr float captionKerningFactor       = 0.1f;
    static constexpr float quaverButtonWidthRatio     = 0.34f;
    static constexpr float ornamentGridCellsPerHeight = 5.0f;

    static constexpr double minimumBindMultiplier  = 0.25;
    static constexpr double maximumBindMultiplier  = 4.0;
    static constexpr double bindMultiplierInterval = 0.25;

    static constexpr int multiplierSliderWidth = 150;
    static constexpr int paintPanelWidth       = 165;
    static constexpr int quaverPaneWidth       = 56;

    NodeCanvas&          nodeCanvas;
    NotePreviewGestures& notePreviewGestures;
    OrnamentLibraries&   ornamentLibraries;

    juce::ValueTree canvasSettings;

    PaintToolSettings      paintPanel;
    ArsNovaUI::ValueEditor countsField;

    ArsNovaUI::ButtonPane toolPane;
    ArsNovaUI::ButtonPane quaverPane;
    ArsNovaUI::ButtonPane bindPane;
    ArsNovaUI::ButtonPane ornamentPane;

    ArsNovaUI::IconButton* quaverTool     = nullptr;
    ArsNovaUI::IconButton* spanTool       = nullptr;
    ArsNovaUI::IconButton* ornamentButton = nullptr;

    BindAxis xAxis { nullptr, {}, ValueTreeIdentifiers::ArrowXBinding, ValueTreeIdentifiers::ArrowXMultiplier, ValueTreeIdentifiers::ArrowXTarget };
    BindAxis yAxis { nullptr, {}, ValueTreeIdentifiers::ArrowYBinding, ValueTreeIdentifiers::ArrowYMultiplier, ValueTreeIdentifiers::ArrowYTarget };
};
