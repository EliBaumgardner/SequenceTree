/*
  ==============================================================================

    ColourWheel.h
    Created: 12 Jun 2025 2:50:27pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class ColourWheel : public juce::Component {
    
    public:
    
    ColourWheel();
    ~ColourWheel() override;
    
    void paint(juce::Graphics& g) override;
    
    void resized() override;
    
    void mouseDown(const juce::MouseEvent& e) override;

    void mouseDrag(const juce::MouseEvent& e) override;
    
    std::function<void(juce::Colour)> onColourPicked;
    
    private:
    
    juce::Colour currentColour;
    juce::Image image;
    
    class ColourField : public juce::DocumentWindow {
        
        public:
        
        juce::Image image;
        
        std::function<void(juce::Colour)> onColourPicked;
        
        ColourField() : juce::DocumentWindow("win",juce::Colours::white, juce::DocumentWindow::allButtons){
            
            setSize(200,200);
            setVisible(true);
        }
        
        void paint(juce::Graphics& g) override {
            
            const int width  = getWidth();
            const int height = getHeight();
            
            image = juce::Image(juce::Image::RGB, width, height, true);
            
            for(int y = 0; y < height; ++y){
                for(int x = 0; x < height; ++y){
                    
                    float hue        = juce::jmap((float)x, 0.0f, (float)width, 0.0f,1.0f);
                    float saturation = juce::jmap((float)y, 0.0f, (float)height, 1.0f,0.0f);
            
                    juce::Colour colour = juce::Colour::fromHSV(hue,saturation,1.0f,1.0f);
            
                    image.setPixelAt(x,y,colour);
                    
                }
            }
            
            g.drawImageAt(image, 0, 0);
        }
        
        void MouseDrag(const juce::MouseEvent& e) {
            
            if(image.isValid() && e.x >= 0 && e.x < getWidth() && e.y >= 0 && e.y < getHeight()){
                
                juce::Colour pickedColour = image.getPixelAt(e.x,e.y);
                onColourPicked(pickedColour);
            }
        }
        
        void closeButtonPressed() override {
            
            delete this;
        }
        
        private:
        
    
    };
    
    class ColourButton : public juce::Component {
        public:
        
        juce::Colour selectedColour;
        std::function<void(bool)> buttonToggled;
        
        ColourButton();
        
        void paint(juce::Graphics& g) override {
            
            g.fillAll(selectedColour);
        }
       
        void mouseDown(const juce::MouseEvent& e) override {
            
            buttonToggled(true);
        }
        
    };
    
    ColourField field;
    ColourButton colourButton;
};
