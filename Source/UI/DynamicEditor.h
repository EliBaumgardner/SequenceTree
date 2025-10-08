/*
  ==============================================================================

    DynamicEditor.h
    Created: 18 May 2025 10:55:47am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "../Util/ProjectModules.h"

class DynamicEditor : public juce::TextEditor {
    
    
    public:
    
        DynamicEditor();
    
        ~DynamicEditor();
    
        void paint(juce::Graphics& g) override;
        
        void refit();

        void bindEditor(juce::ValueTree tree, const juce::Identifier propertyID);
    
        void setEditable(bool editable);
    
        void setLabelText(juce::String string);
        
        void setGroup(juce::String groupID);
    
        void clearBindings();
        
        void makeBoundsVisible(bool isBoundsVisible);
    
        void displayHint(bool showHint, juce::String hintText);
    
        void mouseDown(const juce::MouseEvent& e) override;
    
        enum class DisplayMode {Pitch, Velocity, Duration};
    
        void formatDisplay(DisplayMode mode);
    
        int noteToNumber(juce::String string);
    
        DisplayMode mode;
    
        juce::String hintText = "test";
    
    private:
    
        bool textChanged(juce::String string);
        
        juce::String groupID;
        juce::String labelText;
        juce::String oldString;
        juce::String bindText;
    
        juce::Font baseFont;
    
        juce::Value bindValue;
    
        juce::ValueTree tree;
      
        int numRefits = 0;
    
        bool isBoundsVisible = true;
        bool supressChange = false;
        bool showHint = false;
        bool wasFocused = false;
        bool editable = true;
        bool setOldVal = true;
    
};
