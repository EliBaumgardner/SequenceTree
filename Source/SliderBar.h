/*
  ==============================================================================

    SliderBar.h
    Created: 29 May 2025 12:39:02pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "NodeMenu.h"
//#include "PluginEditor.h"

class PluginEditor;

class SliderBar : public juce::Component {
    
    public:
    
        enum class SliderPosition { Top, Bottom, Left, Right };
    
        SliderBar(SliderPosition position, float thickness);
    
        ~SliderBar() override;
    
        void paint(juce::Graphics& g) override;
    
        void resized() override;
    
        void mouseDrag(const juce::MouseEvent& event) override;
        
        void mouseDown(const juce::MouseEvent& event) override;
    
        void setPosition();
    
        juce::Rectangle<int> getMenuBounds();
    
    private:
        
        int thickness = 0;
    
        juce::Rectangle<int> menuBounds;
        juce::Rectangle<int> border;
        
        SliderPosition sliderPosition;
        juce::Point<int> lastMousePos;
};
