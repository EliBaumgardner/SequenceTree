/*
  ==============================================================================

    NodeTextEditor.h
    Created: 1 Sep 2025 1:28:52pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Util/NodeInfo.h"

class Node;

class NodeCanvas;

class NodeTextEditor : public juce::TextEditor {
    
public:

    NodeTextEditor(Node* node);
    void paint(juce::Graphics& g) override;
    void refit();
    void bindEditor(juce::ValueTree tree, const juce::Identifier propertyID);
    
    enum class DisplayMode {Pitch, Velocity, Duration, CountLimit};
    
    void formatDisplay(NodeDisplayMode mode);
    int noteToNumber(juce::String string);
    
    void makeBoundsVisible(bool isBoundsVisible);
    
    void mouseDrag(const juce::MouseEvent& e) override;
    
    NodeDisplayMode mode;
    juce::Value bindValue;
    
private:
    
    Node* node = nullptr;
    
    juce::Font baseFont;

    juce::ValueTree nodeTree;
};
