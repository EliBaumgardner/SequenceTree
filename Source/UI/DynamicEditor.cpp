/*
  ==============================================================================

    DynamicEditor.cpp
    Created: 18 May 2025 10:55:47am
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "DynamicEditor.h"
#include "../Util/PluginContext.h"
#include "../Node/NodeCanvas.h"


DynamicEditor::DynamicEditor()
{
    setLookAndFeel(ComponentContext::lookAndFeel);

    onTextChange = [this](){
        if(tree.hasType("MidiNoteData")){ ComponentContext::canvas->makeRTGraph(ComponentContext::canvas->root); }
        refit();
    };

    baseFont = juce::Font(getFont());
}

void DynamicEditor::refit()
{
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        
        float length = baseFont.getStringWidthFloat(getText());
        float height = baseFont.getHeight();
        float ratio = std::min(bounds.getWidth()/length,bounds.getHeight()/height);
        
        height *= ratio;
        
        auto font = baseFont;
    
        font.setHeight(height);
        setFont(font);
        applyFontToAllText(font);
}

void DynamicEditor::paint(juce::Graphics& g)
{
    if (auto* customLookAndFeel = dynamic_cast<CustomLookAndFeel*>(&getLookAndFeel())) { customLookAndFeel->drawEditor(g,*this); }
}

void DynamicEditor::mouseDown(const juce::MouseEvent& e)
{
    TextEditor::mouseDown(e);
    onTextChange();
}

void DynamicEditor::formatDisplay(DisplayMode mode)
{
    juce::String displayValue = bindValue.toString();
    double value = displayValue.getDoubleValue();

    juce::String display;
    
    if(mode == DisplayMode::Pitch){
        value += 1;
        int valueRange = (int)value % 127;
        int pitchValue = (int)valueRange % 11;
        int octave = (valueRange+1) / 12;
        
        juce::String pitchNames[] = {
            juce::String(L"C"),
            juce::String(L"C♯"),
            juce::String(L"D"),
            juce::String(L"D♯"),
            juce::String(L"E"),
            juce::String(L"F"),
            juce::String(L"F♯"),
            juce::String(L"G"),
            juce::String(L"G♯"),
            juce::String(L"A"),
            juce::String(L"A♯"),
            juce::String(L"B")
        };
        display = pitchNames[pitchValue] + juce::String(octave);
    }
    
    if(mode == DisplayMode::Velocity){ int velocity = (int)value; display = juce::String(velocity); }
    
    if(mode == DisplayMode::Duration){ display = juce::String(value); }
    
    setText(display);
    refit();
}
