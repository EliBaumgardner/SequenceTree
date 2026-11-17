#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "Processor/PluginProcessor.h"
#include "Editor/Canvas/NodeCanvas.h"
#include "Editor/Canvas/CanvasFrame.h"
#include "Editor/Canvas/NodeLimitWarning.h"
#include "Editor/Nodes/Node.h"
#include "Processor/AudioModel/RTGraphBuilder.h"
#include <ArsNovaUI/Canvas/DynamicPort.h>
#include "Editor/Bars/Titlebar.h"
#include "Editor/Bars/BottomBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Processor/Document/GraphState.h"
#include "Editor/Canvas/GraphDocument.h"
#include "Editor/Menus/MenuArea.h"
#include "Editor/Menus/TraversalMenu.h"
#include "Editor/Menus/NodeMenu.h"
#include "Editor/Input/Gestures/NotePreviewGestures.h"
#include "Editor/Rules/TraversalRulesWindow.h"
#include "Editor/Ornaments/OrnamentWindow.h"
#include <ArsNovaUI/Menus/SettingsMenu.h>
#include <ArsNovaUI/PopupWindow.h>
#include <ArsNovaUI/ScaledView.h>

/*
    The plugin's window, which exists only while it is open. Its constructor builds every UI part
    and gives each one the processor members it uses: the look-and-feel and interface settings, the
    NodeCanvas, the NodeController that routes the canvas's mouse events, the NotePreviewGestures
    that the controller, TraversalMenu and BottomBar share, the DynamicPort that zooms and pans the
    canvas, the TraversalMenu and NodeMenu, the MenuArea, Titlebar and BottomBar, and the
    CanvasFrame that frames them. It owns the traversal-rules, settings and ornament windows and
    wires the buttons that open them.
    It also attaches the canvas's tree listener to the graph, so edits reach the screen.

    audioCommandFrames, a VBlankAttachment, runs once per display frame: it applies a play or stop
    change to the Titlebar and has the canvas drain the AudioUIBridge, which is how playback is
    shown. resized lays the parts out, and configureWindowSize sets the zoom and size limits.
    keyPressed handles the window-wide keys: space toggles playback, delete, undo and redo, copy,
    paste and select all (through nodeController's selectionGestures).

    It holds only views and input. The processor owns everything the sound needs, so destroying
    the editor does not affect playback.
*/

class SequenceTreeAudioProcessor;

class SequenceTreeAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                          public juce::KeyListener
{

public:

    SequenceTreeAudioProcessorEditor (SequenceTreeAudioProcessor&);
    ~SequenceTreeAudioProcessorEditor() override;

    void paint  (juce::Graphics&) override;
    void resized() override;

    void setManualMenuBounds   (juce::Rectangle<int> b);
    void parentHierarchyChanged() override;

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;

private:

    void configureWindowSize();
    void updateResizeLimits ();

    void wireCallbacks      ();

    std::unique_ptr<ArsNovaUI::SettingsMenu> makeSettingsMenu();

    juce::Component* keyListenerTarget = nullptr;

    SequenceTreeAudioProcessor& audioProcessor;

    ArsNovaUI::CustomLookAndFeel lookAndFeel;

    std::unique_ptr<juce::PropertiesFile> interfaceSettings;

    juce::Component workspace;

    std::unique_ptr<ArsNovaUI::ScaledView> scaledView;

    GraphDocument       graphDocument;
    NotePreviewGestures notePreviewGestures;
    NodeLimitWarning    nodeLimitWarning;

    ArsNovaUI::PopupWindowLauncher traversalRulesLauncher {
        "Traversal Rules",
        [this]() {
            auto content = std::make_unique<TraversalRulesWindow>(lookAndFeel, audioProcessor.document.traversalRuleState, audioProcessor.document.undoManager);

            content->setSize(TraversalRulesWindow::defaultWidth, TraversalRulesWindow::defaultHeight);

            return content;
        }
    };

    ArsNovaUI::PopupWindowLauncher settingsLauncher {
        "Settings",
        [this]() {
            auto content = makeSettingsMenu();

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        }
    };

    ArsNovaUI::PopupWindowLauncher ornamentLauncher {
        "Ornaments",
        [this]() {
            auto content = std::make_unique<OrnamentWindow>(lookAndFeel, audioProcessor.document.ornamentLibraries, audioProcessor.document.canvasSettings, audioProcessor.document.ornamentUndoManager);

            const juce::Point<int> size = content->narrowestSize();

            content->setSize(size.x, size.y);

            return content;
        }
    };

public:
    std::unique_ptr<NodeCanvas> nodeCanvas = nullptr;
    std::unique_ptr<Titlebar>   titleBar   = nullptr;

private:
    std::unique_ptr<NodeController>         nodeController = nullptr;
    std::unique_ptr<BottomBar>              bottomBar      = nullptr;
    std::unique_ptr<ArsNovaUI::DynamicPort> port           = nullptr;
    std::unique_ptr<TraversalMenu>          traversalMenu  = nullptr;
    std::unique_ptr<NodeMenu>               nodeMenu       = nullptr;
    std::unique_ptr<MenuArea>               menuArea       = nullptr;
    std::unique_ptr<CanvasFrame>            canvasFrame    = nullptr;

    int menuWidth = 0;

    float menuHeightRatio = 0.25f;

    const int minimumMenuWidth = juce::roundToInt(ArsNovaUI::Theme::barHeight * (MenuArea::menuBarWidthRatio + MenuArea::resizerWidthRatio));

    static constexpr int seedWidth             = 682;
    static constexpr int seedHeight            = 500;
    static constexpr int maximumWindowSide     = 8192;
    static constexpr int barCanvasGap          = 3;
    static constexpr int horizontalGutterCount = 3;
    static constexpr int verticalGutterCount   = 2;

    static constexpr float openingScale = 2.0f;

    juce::VBlankAttachment audioCommandFrames;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessorEditor)
};
