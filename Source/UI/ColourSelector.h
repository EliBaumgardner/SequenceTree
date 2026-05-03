/*
  ==============================================================================

    ColourSelector.h
    Created: 10 Aug 2025 5:05:50pm
    Author:  Eli Baimgardner

  ==============================================================================
*/

#pragma once


#include "../Util/PluginModules.h"
#include "../Util/ApplicationContext.h"
#include "Node/Node.h"

/* Colour Selector Button that displays the selected colour and opens a pop up menu and cursor for colour selection; */


class Cursor : public juce::Component
{
public:
    void paint(juce::Graphics& g) override;
};

class PresetSwatch : public juce::Component
{
public:
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;

    juce::Colour colour { juce::Colours::lightgrey };
    bool isSet = false;
    std::function<void(juce::Colour)> onApply;
    std::function<void()>             onSave;
};

class MainComponent : public juce::Component
{
public:
    static constexpr int numPresets     = 8;
    static constexpr int presetRowHeight = 22;

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

    static juce::Colour presetColours[numPresets];
    static bool         presetSet[numPresets];

private:
    juce::OwnedArray<PresetSwatch> swatches;
};

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

class ColourSelector : public juce::Component {

    public:

    ColourSelector(ApplicationContext& context);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void setNode(Node* node);


    juce::Colour colour = juce::Colours::white;
    std::unique_ptr<MainWindow> mainWindow = nullptr;
    Node* node = nullptr;

private:
    ApplicationContext& applicationContext;
    void applyColourToDescendants(Node* n, juce::Colour c);
};
