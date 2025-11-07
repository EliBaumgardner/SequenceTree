/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "../CustomLookAndFeel.h"
#include "DynamicEditor.h"
#include "../Util/ProjectModules.h"
#include "../Util/ComponentContext.h"

class PlayButton : public juce::Button {

public:

    PlayButton() : juce::Button("button") { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawPlayButton(g,isMouseOver,isButtonDown,*this); }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Button::mouseDown(event);
        isOn = !isOn;
        repaint();
    }

    bool isOn = true;
};

class SyncButton : public juce::Button {

public:
    SyncButton() : juce::Button("button") { setLookAndFeel(ComponentContext::lookAndFeel); }

    void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
    {
        if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawSyncButton(g, isMouseOver, isButtonDown, *this); }
    }

    void mouseDown(const juce::MouseEvent& event) override
    {
        juce::Button::mouseDown(event);
        isSynced = !isSynced;
        repaint();
    }

    bool isSynced = true;
};

class TitleBar : public juce::Component {
    
    public:
    
        TitleBar();
        
        void paint(juce::Graphics& g) override;
        void resized() override;
    
        std::function<void()> toggled;
    
    private:

    DynamicEditor editor;

    PlayButton playButton;
    
    juce::ToggleButton toggleButton;
    
    SyncButton syncButton;
};
