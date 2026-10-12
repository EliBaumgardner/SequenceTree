/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include "PluginProcessor.h"
#include "../UI/Canvas/NodeCanvas.h"
#include "../UI/Canvas/CanvasFrame.h"
#include "../UI/Node/Node.h"
#include "../Graph/RTGraphBuilder.h"
#include "../UI/Canvas/DynamicPort.h"
#include "../UI/Bars/Titlebar.h"
#include "../UI/Bars/BottomBar.h"
#include "../UI/Theme/CustomLookAndFeel.h"
#include "../Graph/GraphState.h"
#include "../UI/Menus/MenuArea.h"
#include "../UI/ScaledView.h"


//==============================================================================

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

    SequenceTreeAudioProcessor&           audioProcessor;
    CustomLookAndFeel                     lookAndFeel;
    std::unique_ptr<juce::PropertiesFile> interfaceSettings;
    juce::Component                       workspace;
    std::unique_ptr<ScaledView>           scaledView;
    juce::TooltipWindow                   tooltipWindow { &workspace, 400 };

public:
    std::unique_ptr<NodeCanvas> nodeCanvas = nullptr;
    std::unique_ptr<Titlebar>   titleBar   = nullptr;

private:
    std::unique_ptr<NodeController> nodeController = nullptr;
    std::unique_ptr<BottomBar>      bottomBar      = nullptr;
    std::unique_ptr<DynamicPort>    port           = nullptr;
    std::unique_ptr<MenuArea>       menuArea       = nullptr;
    std::unique_ptr<CanvasFrame>    canvasFrame    = nullptr;

    int   menuWidth       = 0;
    float menuHeightRatio = 0.25f;

    static constexpr int   seedWidth         = 682;
    static constexpr int   seedHeight        = 500;
    static constexpr int   maximumWindowSide = 8192;
    static constexpr float openingScale      = 2.0f;

    juce::VBlankAttachment audioCommandFrames;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SequenceTreeAudioProcessorEditor)
};
