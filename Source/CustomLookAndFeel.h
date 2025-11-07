//
// Created by Eli Baumgardner on 11/4/25.
//

#ifndef SEQUENCETREE_CUSTOMLOOKANDFEEL_H
#define SEQUENCETREE_CUSTOMLOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>

class NodeCanvas;
class Node;
class NodeArrow;

class TitleBar;
class SelectionBar;

class PlayButton;
class SyncButton;

class DynamicEditor;

class CustomLookAndFeel : public juce::LookAndFeel_V4
{

public:

    CustomLookAndFeel();

    virtual void drawEditor       (juce::Graphics& g, DynamicEditor& editor);
    virtual void drawCanvas       (juce::Graphics& g, const NodeCanvas& canvas);

    virtual void drawTitleBar     (juce::Graphics& g, const TitleBar& titleBar);
    virtual void MenuBar          (juce::Graphics& g, const SelectionBar& selectionBar);

    virtual void drawNodeSelector (juce::Graphics& g);
    virtual void drawDisplaySelector(juce::Graphics& g);

    virtual void drawNode         (juce::Graphics& g, const Node& node);
    virtual void drawNodeArrow    (juce::Graphics& g, const NodeArrow& nodeArrow);

    virtual void drawPlayButton   (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const PlayButton& button);
    virtual void drawSyncButton   (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const SyncButton& button);

private:

    juce::Colour dropShadowColour = juce::Colours::black;
    juce::Colour darkColour1  = juce::Colour::fromRGB(40,40,38);
    juce::Colour darkColour2  = juce::Colour::fromRGB(30,30,30);
    juce::Colour lightColour1 = juce::Colour::fromRGB(195,174,132);
    juce::Colour lightColour2 = juce::Colour::fromRGB(162,150,131);
    juce::Colour lightColour3 = juce::Colour::fromRGB(217,217,217);

    juce::Colour canvasColour = darkColour1;
    juce::Colour barColour    = darkColour2;
    juce::Colour buttonColour = lightColour2;
    juce::Colour editorColour = lightColour2;
    juce::Colour textColour   = lightColour3;
    juce::Colour arrowColour  = juce::Colours::black;

    juce::DropShadow barDropShadow;
    juce::DropShadow nodeDropShadow;
};

#endif //SEQUENCETREE_CUSTOMLOOKANDFEEL_H