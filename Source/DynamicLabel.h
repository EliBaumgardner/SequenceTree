/*
  ==============================================================================

    DynamicLabel.h
    Created: 28 May 2025 2:28:09pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once

#include "JuceHeader.h"

class DynamicLabel : public juce::Label {
    
    using juce::Label::Label;
    
    public:
    
        void refit();
    
    private:
        
        int numRefits = 0;
};
