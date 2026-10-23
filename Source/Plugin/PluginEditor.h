#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Canvas/CanvasFrame.h"
#include "../UI/Node/Node.h"
#include "../Graph/RTGraphBuilder.h"
#include <ArsNovaUI/Canvas/DynamicPort.h>
#include "../UI/Bars/Titlebar.h"
#include "../UI/Bars/BottomBar.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Graph/GraphState.h"
#include "../UI/Menus/MenuArea.h"
#include <ArsNovaUI/ScaledView.h>

/*
    The plugin's window, which exists only while it is open. Its constructor builds every UI part
    and gives each one the processor members it uses: the look-and-feel and interface settings, the
    NodeCanvas, the NodeController that routes the canvas's mouse events, the DynamicPort that
    zooms and pans it, the MenuArea, Titlebar and BottomBar, and the CanvasFrame that frames them.
    It also attaches the canvas's tree listener to the graph, so edits reach the screen.

    audioCommandFrames, a VBlankAttachment, runs once per display frame: it applies a play or stop
    change to the Titlebar and has the canvas drain the AudioUIBridge, which is how playback is
    shown. resized lays the parts out, and configureWindowSize sets the zoom and size limits.
    keyPressed handles the window-wide keys: space toggles playback, delete, undo and redo, copy,
    paste and select all (through nodeController's selectionGestures), and full screen in the
    standalone app.

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
    void toggleFullScreen   ();

    juce::Component* keyListenerTarget = nullptr;

    SequenceTreeAudioProcessor&            audioProcessor;
    ArsNovaUI::CustomLookAndFeel           lookAndFeel;
    std::unique_ptr<juce::PropertiesFile>  interfaceSettings;
    juce::Component                        workspace;
    std::unique_ptr<ArsNovaUI::ScaledView> scaledView;
    juce::TooltipWindow                    tooltipWindow { &workspace, 400 };

public:
    std::unique_ptr<NodeCanvas> nodeCanvas = nullptr;
    std::unique_ptr<Titlebar>   titleBar   = nullptr;

private:
    std::unique_ptr<NodeController>         nodeController = nullptr;
    std::unique_ptr<BottomBar>              bottomBar      = nullptr;
    std::unique_ptr<ArsNovaUI::DynamicPort> port           = nullptr;
    std::unique_ptr<MenuArea>               menuArea       = nullptr;
    std::unique_ptr<CanvasFrame>            canvasFrame    = nullptr;

    int   menuWidth       = 0;
    float menuHeightRatio = 0.25f;

    static constexpr int   seedWidth         = 682;
    static constexpr int   seedHeight        = 500;
    static constexpr int   maximumWindowSide = 8192;
    static constexpr float openingScale      = 2.0f;

    juce::VBlankAttachment audioCommandFrames;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessorEditor)
};
