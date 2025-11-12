/*
  ==============================================================================

    DynamicEditor.h
    Created: 18 May 2025 10:55:47am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "../Util/PluginContext.h"
#include "../Util/PluginModules.h"

class DynamicEditor : public juce::TextEditor {
    
    public:
    
        DynamicEditor();
        void paint(juce::Graphics& g) override;
        void refit();
        void mouseDown(const juce::MouseEvent& e) override;
    
        enum class DisplayMode {Pitch, Velocity, Duration};
    
        void formatDisplay(DisplayMode mode);

        DisplayMode mode;

    private:

        juce::Font baseFont;
        juce::Value bindValue;
        juce::ValueTree tree;
      
        int numRefits = 0;
};
