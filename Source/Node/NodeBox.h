/*
  ==============================================================================

    NodeBox.h
    Created: 1 Sep 2025 1:28:52pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "../Util/PluginModules.h"

class Node;

class NodeCanvas;

class NodeBox : public juce::TextEditor {
    
    public:
    
    NodeBox(Node* node);
    void paint(juce::Graphics& g) override;
    void refit();
    void bindEditor(juce::ValueTree tree, const juce::Identifier propertyID);
    
    enum class DisplayMode {Pitch, Velocity, Duration, CountLimit};
    
    void formatDisplay(DisplayMode mode);
    int noteToNumber(juce::String string);
    
    void makeBoundsVisible(bool isBoundsVisible);
    
    void mouseDrag(const juce::MouseEvent& e) override;
    
    DisplayMode mode;
    juce::Value bindValue;
    
    private:
    
    Node* node = nullptr;
    
    juce::Font baseFont;

    juce::ValueTree tree;
    
};
