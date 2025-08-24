/*
  ==============================================================================

    TitleBar.cpp
    Created: 12 Jun 2025 6:50:43pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#include "TitleBar.h"

TitleBar::TitleBar(){
    addAndMakeVisible(playButton);
    addAndMakeVisible(editor);

    playButton.onClick = [=](){
        toggled();
    };
}

TitleBar::~TitleBar(){
    
}

void TitleBar::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::white);
    g.setColour(juce::Colours::black);

    auto inner = getBounds().reduced(4);
    g.drawRect(inner,1.0f);
    
}

void TitleBar::resized(){
    
    
    auto playButtonWidth = getBounds().getWidth()/25;
    auto editorWidth = getBounds().getWidth()/15;
    
    playButton.setBounds(getWidth()/2 - (playButtonWidth/2+editorWidth),getHeight()*0.2,playButtonWidth,getHeight()*0.6);
    editor.setBounds(getWidth()/2 - (editorWidth/2),getHeight()*0.2,editorWidth,getHeight()*0.6);
    editor.refit();
    
    //sync button
}
