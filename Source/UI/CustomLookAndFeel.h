//
// Created by Eli Baumgardner on 11/4/25.
//

#ifndef SEQUENCETREE_CUSTOMLOOKANDFEEL_H
#define SEQUENCETREE_CUSTOMLOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>

class NodeCanvas;

class Node;
class RootNode;
class RootRectangle;

class NodeArrow;

class Titlebar;
class ButtonPane;

class DisplayMenu;
class DisplayButton;

class NodeButton;
class ConnectorButton;
class ModulatorButton;

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

    static CustomLookAndFeel& get(juce::Component& c)
    {
        return static_cast<CustomLookAndFeel&>(c.getLookAndFeel());
    }

    void drawEditor         (juce::Graphics& g, CustomTextEditor& editor);
    void drawNodeTextEditor (juce::Graphics& g, NodeTextEditor& editor);
    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;

    void drawCanvas         (juce::Graphics& g, const NodeCanvas& canvas);

    void drawTitleBar       (juce::Graphics& g, const Titlebar& titleBar);

    void drawDisplayMenu    (juce::Graphics& g, const DisplayMenu& displaySelector);
    void drawButtonPane     (juce::Graphics& g, const ButtonPane& selectionBar);
    void drawDisplayButton  (juce::Graphics& g, const DisplayButton& displayButton);

    void drawNodeInteractionEffects(juce::Graphics &g, const Node &node, juce::Rectangle<float> bounds);

    void drawNode           (juce::Graphics& g, const Node& node);
    void drawNode           (juce::Graphics& g, const Node& node, juce::Rectangle<float> circleBoundsOverride);
    void drawRootNode       (juce::Graphics& g, const RootNode& node);
    void drawRootNodeRectangle(juce::Graphics& g, const RootRectangle& rootRectangle);

    void drawNodeArrowText          (juce::Graphics &g, const NodeArrow &nodeArrow, const juce::TextEditor &editor, TextCords textCords);

    void drawNodeArrow      (juce::Graphics& g, const NodeArrow& nodeArrow, const juce::TextEditor& editor);

    void drawPlayButton     (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const PlayButton& button);
    void drawSyncButton     (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const SyncButton& button);

    void drawNodeButton     (juce::Graphics& g, const NodeButton& nodeButton);
    void drawTraverserButton(juce::Graphics& g, const ConnectorButton& traverserButton);
    void drawModulatorButton(juce::Graphics& g, const ModulatorButton& modulatorButton);

    void drawTempoDisplay   (juce::Graphics& g, const TempoDisplay& tempoDisplay);

    void drawUndoButton     (juce::Graphics& g, const UndoButton& undoButton, bool isButtonDown);
    void drawRedoButton     (juce::Graphics& g, const RedoButton& redoButton, bool isButtonDown);
    void drawUndoRedoPane   (juce::Graphics& g, const UndoRedoPane& undoRedoPane);
    void drawResetButton    (juce::Graphics& g, const ResetButton& resetButton, bool isButtonDown);


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
    juce::Colour arrowColour  = darkColour2.darker();
    juce::Colour arrowHeadColour = arrowColour;

    juce::DropShadow barDropShadow;
    juce::DropShadow nodeDropShadow;
};

#endif //SEQUENCETREE_CUSTOMLOOKANDFEEL_H