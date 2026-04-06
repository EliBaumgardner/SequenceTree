//
// Created by Eli Baumgardner on 11/4/25.
//

#ifndef SEQUENCETREE_CUSTOMLOOKANDFEEL_H
#define SEQUENCETREE_CUSTOMLOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>

class NodeCanvas;
class Node;
class NodeArrow;

class Titlebar;
class ButtonPane;

class DisplayMenu;
class DisplayButton;

class NodeButton;
class ConnectorButton;

class PlayButton;

class TempoDisplay;
class SyncButton;

class CustomTextEditor;
class NodeTextEditor;

class UndoButton;
class RedoButton;

class UndoRedoPane;
class ResetButton;

class CustomLookAndFeel : public juce::LookAndFeel_V4
{

public:

    struct TextCords {
        int parentNodeX;
        int parentNodeY;
        int childNodeX;
        int childNodeY;
        int newX;
        int newY;
    };

    CustomLookAndFeel();

    virtual void drawEditor         (juce::Graphics& g, CustomTextEditor& editor);
    virtual void drawNodeTextEditor (juce::Graphics& g, NodeTextEditor& editor);
    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;

    virtual void drawCanvas         (juce::Graphics& g, const NodeCanvas& canvas);

    virtual void drawTitleBar       (juce::Graphics& g, const Titlebar& titleBar);

    virtual void drawDisplayMenu    (juce::Graphics& g, const DisplayMenu& displaySelector);
    virtual void drawButtonPane     (juce::Graphics& g, const ButtonPane& selectionBar);
    virtual void drawDisplayButton  (juce::Graphics& g, const DisplayButton& displayButton);

    virtual void drawNode           (juce::Graphics& g, const Node& node);

    void drawNodeArrowText          (juce::Graphics &g, const NodeArrow &nodeArrow, const juce::TextEditor &editor, TextCords textCords);

    virtual void drawNodeArrow      (juce::Graphics& g, const NodeArrow& nodeArrow, const juce::TextEditor& editor);

    virtual void drawPlayButton     (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const PlayButton& button);
    virtual void drawSyncButton     (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const SyncButton& button);
    virtual void drawNodeButton     (juce::Graphics& g, const NodeButton& nodeButton);
    virtual void drawTraverserButton(juce::Graphics& g, const ConnectorButton& traverserButton);
    virtual void drawTempoDisplay   (juce::Graphics& g, const TempoDisplay& tempoDisplay);

    virtual void drawUndoButton     (juce::Graphics& g, const UndoButton& undoButton, bool isButtonDown);
    virtual void drawRedoButton     (juce::Graphics& g, const RedoButton& redoButton, bool isButtonDown);
    virtual void drawUndoRedoPane   (juce::Graphics& g, const UndoRedoPane& undoRedoPane);
    virtual void drawResetButton    (juce::Graphics& g, const ResetButton& resetButton, bool isButtonDown);


private:

    juce::Colour dropShadowColour = juce::Colours::black;
    juce::Colour darkColour1      = juce::Colour::fromRGB(40,40,38);
    juce::Colour darkColour2      = juce::Colour::fromRGB(30,30,30);
    juce::Colour lightColour1     = juce::Colour::fromRGB(195,174,132);
    juce::Colour lightColour2     = juce::Colour::fromRGB(162,150,131);
    juce::Colour lightColour3     = juce::Colour::fromRGB(217,217,217);

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