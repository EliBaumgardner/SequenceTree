/*
  ==============================================================================

    ColourSelector.h
    Created: 10 Aug 2025 5:05:50pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once


#include "ProjectModules.h"
#include "Node.h"

/* Colour Selector Button that displays the selected colour and opens a pop up menu and cursor for colour selection; */


//cursor for pop up menu
class Cursor : public juce::Component
{
    public:
    
    void paint(juce::Graphics& g) override;

    
};

//pop up menu component
class MainComponent : public juce::Component
{
    public:
    
    MainComponent();
    
    void paint(juce::Graphics& g) override;
    void resized() override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void generateImage();
    void updateCursorPosition(juce::Colour selectedColour);
    
    std::function<void(juce::Colour)> colourPicked;
    
    juce::Colour colour = juce::Colours::white;
    Cursor cursor;
    juce::Image image;
};

//component for opening and closing pop up menu component
class MainWindow : public juce::DocumentWindow
{
    public:
    
    MainWindow (const juce::String& name,
                    juce::Colour backgroundColour,
                    int requiredButtons,
                    bool addToDesktop = true);
    
    void closeButtonPressed() override;
    
    MainComponent* getContent();
    
    MainComponent* component = nullptr;
};

//button that opens, closes, and displays pop up menu
class ColourSelector : public juce::Component {
    
    public:
    
    ColourSelector();
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void setNode(Node* node);
    
    
    juce::Colour colour = juce::Colours::white;
    std::unique_ptr<MainWindow> mainWindow = nullptr;
    Node* node = nullptr;
};
