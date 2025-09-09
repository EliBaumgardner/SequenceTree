#include "PluginEditor.h"
#include "SliderBar.h"
#include <JuceHeader.h>

SliderBar::SliderBar(SliderPosition position, float thickness) : thickness(thickness),sliderPosition(position){
    //resized();
    
}

SliderBar::~SliderBar(){
    
}

void SliderBar::resized(){
    
    auto parentBounds = getParentComponent()->getLocalBounds();
    
    
    switch(sliderPosition){
            
        case SliderPosition::Top:
            setBounds(parentBounds.removeFromTop(thickness));
            break;
            
        case SliderPosition::Bottom: setBounds(parentBounds.removeFromBottom(thickness));
            break;
            
        case SliderPosition::Left: setBounds(parentBounds.removeFromLeft(thickness)); break;
            
        case SliderPosition::Right: setBounds(parentBounds.removeFromRight(thickness)); break;
            
            
//            border = getBounds().reduced(2.0f);
    }
        
}

void SliderBar::mouseDown(const juce::MouseEvent& event)
{
    lastMousePos = event.getScreenPosition();
}

void SliderBar::mouseDrag(const juce::MouseEvent& event)
{
    if (auto* parent = getParentComponent())
    {
        auto* grandParent = parent->getParentComponent(); // outer container
        if (!grandParent) return;

        auto currentPos = event.getScreenPosition();
        auto delta = currentPos - lastMousePos;

        auto parentBounds = parent->getBounds();
        auto containerBounds = grandParent->getLocalBounds();

        switch (sliderPosition)
        {
            case SliderPosition::Top:
            {
                int newHeight = parentBounds.getHeight() - delta.y;
                int maxHeight = containerBounds.getHeight();
                newHeight = juce::jlimit(thickness, maxHeight, newHeight);

                int newY = containerBounds.getBottom() - newHeight;

                parent->setBounds(parentBounds.getX(),
                                  newY,
                                  parentBounds.getWidth(),
                                  newHeight);
                break;
            }
            case SliderPosition::Bottom:
            {
                int newHeight = parentBounds.getHeight() + delta.y;
                int maxHeight = containerBounds.getHeight();
                newHeight = juce::jlimit(thickness, maxHeight, newHeight);
                parent->setBounds(parentBounds.withHeight(newHeight));
                break;
            }
            case SliderPosition::Left:
            {
                int newWidth = parentBounds.getWidth() + delta.x;
                int maxWidth = containerBounds.getWidth();
                newWidth = juce::jlimit(thickness, maxWidth, newWidth);
                parent->setBounds(parentBounds.withWidth(newWidth));
                break;
            }
            case SliderPosition::Right:
            {
                int newWidth = parentBounds.getWidth() + delta.x;
                int maxWidth = containerBounds.getWidth();
                newWidth = juce::jlimit(thickness, maxWidth, newWidth);
                parent->setBounds(parentBounds.withWidth(newWidth));
                break;
            }
        }
        
        lastMousePos = currentPos;
        menuBounds = parent->getBounds();
        if (auto* editor = dynamic_cast<SequenceTreeAudioProcessorEditor*>(grandParent))
            editor->setManualMenuBounds (parent->getBounds());
    }
}



void SliderBar::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::white); // Background

    auto bounds = getLocalBounds();
    g.setColour(juce::Colours::black);
    g.drawRect(bounds, 2); // Draw white border around full bounds

    // Now draw filled inner area (optional)
    auto inner = bounds.reduced(4); // Some inner area
    g.setColour(juce::Colours::white);
    g.fillRect(inner);
}

void SliderBar::setPosition(){
    
}

juce::Rectangle<int> SliderBar::getMenuBounds(){
    return menuBounds;
}
