/*
  ==============================================================================

    ColourSelector.h
    Created: 10 Aug 2025 5:05:50pm
    Author:  Eli Baumgardner

  ==============================================================================
*/

#pragma once


#include <unordered_set>
#include "../../Util/PluginModules.h"
#include "../../Util/ApplicationContext.h"
#include "../Node/Node.h"
#include "../PopupWindow.h"


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

class ColourSelector : public juce::Component, public juce::SettableTooltipClient {

    public:

    ColourSelector(ApplicationContext& context);
    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void setNode(Node* node);


    juce::Colour colour = juce::Colours::white;
    Node* node = nullptr;

    static constexpr int pickerWidth  = 160;
    static constexpr int pickerHeight = 145;

    bool requiresNode = true;

    std::function<void(juce::Colour)> onColourPicked;

private:
    ApplicationContext& applicationContext;

    PopupWindowLauncher pickerLauncher {
        "",
        []() {
            auto content = std::make_unique<MainComponent>();
            content->setSize(pickerWidth, pickerHeight);

            return content;
        },
        juce::Colours::white
    };

    void applyColourToDescendants(Node* n, juce::Colour c);
    void applyColourToDescendants(Node* n, juce::Colour c, std::unordered_set<int>& visited);
};
