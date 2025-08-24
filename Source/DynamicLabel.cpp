/*
  ==============================================================================

    DynamicLabel.cpp
    Created: 28 May 2025 2:28:09pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "DynamicLabel.h"

void DynamicLabel::refit(){
    
    numRefits++;
    
    if(numRefits % 2 == 0){
        
        auto bounds = getLocalBounds().toFloat().reduced(4.0f);
        
        float length = getFont().getStringWidthFloat(getText());
        float height = getFont().getHeight();
        float ratio = std::min(bounds.getWidth()/length,bounds.getHeight()/height);
        
        height *= ratio;
        
        auto font = getFont();
        font.setHeight(height);
        setFont(font);
        
        repaint();
        
        numRefits = 0;
    }
}



