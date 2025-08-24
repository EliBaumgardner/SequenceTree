/*
  ==============================================================================

    ColourWheel.cpp
    Created: 12 Jun 2025 2:50:27pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "ColourWheel.h"

ColourWheel::ColourWheel(){
    
    addAndMakeVisible(colourButton);
    
}

ColourWheel::~ColourWheel(){
    
}

void ColourWheel::paint(juce::Graphics& g)
{
    
}

void ColourWheel::resized(){
    
    colourButton.setBounds(getBounds());
}

//void ColourWheel::mouseDrag(const juce::MouseEvent& e)
//{
//    if (image.isValid()
//        && e.x >= 0 && e.x < image.getWidth()
//        && e.y >= 0 && e.y < image.getHeight())
//    {
//        juce::Colour pickedColour = image.getPixelAt(e.x, e.y);
//        
//        if (onColourPicked)
//            onColourPicked(pickedColour);
//    }
//}

void ColourWheel::mouseDown(const juce::MouseEvent& e){
    
    
}
