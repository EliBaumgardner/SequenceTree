/*
  ==============================================================================

    NodeMenu.h
    Created: 6 May 2025 8:37:51pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>
#include "DynamicEditor.h"
#include "DynamicField.h"
#include "DynamicLabel.h"
#include "SliderBar.h"
#include "MenuItem.h"
#include "ColourWheel.h"
#include "ColourSelector.h"

class Node;

class NodeData;

class NodeMenu : public juce::Component {
public:
    NodeMenu();
    ~NodeMenu() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setDisplayedNode(Node* displayedNode);
    void setMidiDisplay();
    
    std::unique_ptr<SliderBar> sliderBar;

private:
    
    bool openCC = false;
    bool openNote = false;

    Node* displayedNode = nullptr;
    
    std::unique_ptr<ColourSelector> selector = nullptr;

    DynamicEditor label;
    DynamicEditor nameEditor;

    juce::Colour nodeMenuColour = juce::Colours::white;

    std::unique_ptr<DynamicField> nodeDataField;

    std::unique_ptr<MenuItem> menuRoot;
    std::unique_ptr<MenuItem> midiCCItem = nullptr;
    std::unique_ptr<MenuItem> midiNoteItem = nullptr;

    juce::Array<std::unique_ptr<DynamicField>> midiCCFields;
    juce::Array<std::unique_ptr<DynamicField>> midiNoteFields;
    
    JUCE_LEAK_DETECTOR (NodeMenu)

};

