#pragma once

#include <ArsNovaUI/Bars/Bar.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Editors/ValueEditor.h>
#include "Editor/Paint/PaintToolSettings.h"
#include "Editor/ArrowSettings/BindPane.h"
#include "Editor/Ornaments/Marks/OrnamentPane.h"
#include <ArsNovaUI/Buttons/ValueSlider.h>
#include <ArsNovaUI/Menus/ContextMenu.h>

#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

#include "Core/Model/ArrowInfo.h"
#include "Core/Model/NodeInfo.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

#include <array>

/*
    The bar along the bottom of the editor, holding the controls for the canvas tools: the paint
    button, a note preview button, a span tool button, an ornament tool button, and two buttons
    that bind an arrow's x and y extent to pitch or duration. It is a Bar drawn as frosted glass
    over the NodeCanvas, with the controls in groups divided by hairlines: paint (the paint
    button), tools (note, ornament and span) and bind (the two bind buttons).

    Each tool button toggles NodeCanvas::activeTool, and the bar sets which of them looks selected
    from NodeCanvas::onActiveToolChanged. Right-clicking the paint button opens PaintToolSettings
    as a menu holding the brush colour, size and rate. Right-clicking the note preview button opens
    a menu to repeat the preview, which NotePreviewGestures::toggleRepeat sends to the audio thread,
    and a count field that sets NotePreviewGestures::count by typing a number or scrolling.
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

    int narrowestWidth();

    OrnamentPane ornamentPane;

private:

    ArsNovaUI::Bar::Layout makeLayout();

    void configureCountField();
    void showPaintMenu      ();
    void showQuaverMenu     ();

    static constexpr float toolInsetRatio = 0.05f;

    static constexpr int countFieldWidth = 150;
    static constexpr int countLabelWidth = 40;
    static constexpr int countLabelGap   = 6;

    NodeCanvas&          nodeCanvas;
    NotePreviewGestures& notePreviewGestures;

    PaintToolSettings        paintPanel;
    ArsNovaUI::LabeledEditor countField;

    ArsNovaUI::ButtonPane paintPane;
    ArsNovaUI::ButtonPane toolPane;
    ArsNovaUI::ButtonPane quaverPane;

    BindPane bindPane;

    ArsNovaUI::IconButton* paintTool  = nullptr;
    ArsNovaUI::IconButton* quaverTool = nullptr;
    ArsNovaUI::IconButton* spanTool   = nullptr;
};
