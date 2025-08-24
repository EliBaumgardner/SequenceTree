/*
  ==============================================================================

    TitleBar.h
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DynamicEditor.h"



class TitleBar : public juce::Component {
    
    public:
    
        TitleBar();
        ~TitleBar() override;
        
        void paint(juce::Graphics& g) override;
        void resized() override;
    
        std::function<void()> toggled;
    
    private:
    
    class PlayButton : public juce::Button {
        
        public:
        
        PlayButton() : juce::Button("button") {}
        
        void paintButton(juce::Graphics& g, bool isMouseOver, bool isButtonDown) override
        {
            auto area = getLocalBounds().reduced(5); // stay inside our component bounds
            g.setColour(juce::Colours::black);

            if (isOn)
            {
                // Draw play triangle ( > )
                juce::Path playButton;
                playButton.startNewSubPath((float)area.getX(), (float)area.getY());
                playButton.lineTo((float)area.getRight(), (float)area.getCentreY());
                playButton.lineTo((float)area.getX(), (float)area.getBottom());
                playButton.closeSubPath(); // complete the triangle
                g.fillPath(playButton); // use fill instead of stroke for solid arrow
            }
            else
            {
                // Draw pause bars ( | | )
                int barWidth = area.getWidth() / 5;
                int gap = barWidth;
                int barHeight = area.getHeight();
                int x1 = area.getX() + barWidth;
                int x2 = x1 + barWidth + gap;

                juce::Rectangle<int> leftBar(x1, area.getY(), barWidth, barHeight);
                juce::Rectangle<int> rightBar(x2, area.getY(), barWidth, barHeight);

                g.fillRect(leftBar);
                g.fillRect(rightBar);
            }
        }

        
        void mouseDown(const juce::MouseEvent& event) override {
            juce::Button::mouseDown(event);
            isOn = !isOn;
            repaint();
        }
        
        private:
        
        bool isOn = true;
        
    };
    
    PlayButton playButton;
    
    DynamicEditor editor;
    
    juce::ToggleButton toggleButton;
    
    juce::ToggleButton syncButton;
    
};
