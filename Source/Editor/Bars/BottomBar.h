#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include "PaintToolSettings.h"
#include "BindPane.h"
#include "Editor/Ornaments/OrnamentPane.h"
#include <ArsNovaUI/Buttons/ValueSlider.h>
#include <ArsNovaUI/Menus/ContextMenu.h>

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include "Core/Model/ArrowInfo.h"
#include "Core/Model/NodeInfo.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

#include <array>

/*
    The bar along the bottom of the editor, holding the controls for the canvas tools: the paint
    settings, a note preview button with a counts field, a span tool button, an ornament tool
    button, and two buttons that bind an arrow's x and y extent to pitch or duration. It is a
    Bar drawn as frosted glass over the NodeCanvas.

    Each tool button toggles NodeCanvas::activeTool, and the bar sets which of them looks selected
    from NodeCanvas::onActiveToolChanged. Right-clicking the note preview button opens a menu to
    repeat the preview, which NotePreviewGestures::toggleRepeat sends to the audio thread.
    The ornament button and the two bind buttons are the panes OrnamentPane and BindPane, which
    BottomBar lays out. Right-clicking the ornament button calls OrnamentPane::onWindowRequested,
    which PluginEditor answers by rescanning the OrnamentLibraries and opening the OrnamentWindow;
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


class BottomBar : public ArsNovaUI::Bar
{
public:

    BottomBar(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, juce::ValueTree colourPresets, OrnamentLibraries& ornamentLibraries);

    void resized() override;

    void applyDisplayMode(NodeInfo::DisplayMode mode);

    int narrowestWidth() const;

    OrnamentPane ornamentPane;

private:

    void showQuaverMenu();

    static constexpr float captionHeightRatio     = 0.73f;
    static constexpr float captionKerningFactor   = 0.1f;
    static constexpr float quaverButtonWidthRatio = 0.34f;

    static constexpr int paintPanelWidth = 165;
    static constexpr int quaverPaneWidth = 56;

    NodeCanvas&          nodeCanvas;
    NotePreviewGestures& notePreviewGestures;

    PaintToolSettings      paintPanel;
    ArsNovaUI::ValueEditor countsField;

    ArsNovaUI::ButtonPane toolPane;
    ArsNovaUI::ButtonPane quaverPane;

    BindPane bindPane;

    ArsNovaUI::IconButton* quaverTool = nullptr;
    ArsNovaUI::IconButton* spanTool   = nullptr;
};
