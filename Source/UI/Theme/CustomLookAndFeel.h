//
// Created by Eli Baumgardner on 11/4/25.
//

#ifndef SEQUENCETREE_CUSTOMLOOKANDFEEL_H
#define SEQUENCETREE_CUSTOMLOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

class NodeCanvas;

class Node;
class RootNode;
class RootRectangle;

class NodeArrow;

class NodeButton;
class ModulatorButton;
class TraversalFlagButton;

class PlayButton;
class SyncButton;

class CustomTextEditor;
class NodeTextEditor;

class UndoButton;
class RedoButton;

class UndoRedoPane;
class ResetButton;

class PaintTool;

class PaintToolSettings;

class DanglingArrow;
class ArrowTool;

class IconButton;

class CustomLookAndFeel : public juce::LookAndFeel_V4, public Theme
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

    static CustomLookAndFeel& get(juce::Component& c) { return static_cast<CustomLookAndFeel&>(c.getLookAndFeel()); }

    void drawEditor         (juce::Graphics& g, CustomTextEditor& editor);
    void drawNodeTextEditor (juce::Graphics& g, NodeTextEditor& editor);
    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;

    void drawCanvas         (juce::Graphics& g, const NodeCanvas& canvas);


    void drawNodeIcon       (juce::Graphics& g, const IconButton& iconButton);
    void drawTreeIcon       (juce::Graphics& g, const IconButton& iconButton);
    void drawTraversalIcon   (juce::Graphics& g, const IconButton& iconButton);






    void drawNodeInteractionEffects(juce::Graphics &g, const Node &node, juce::Rectangle<float> bounds);

    static juce::Rectangle<float> getNodeCircleBounds(juce::Rectangle<float> componentBounds);

    void drawNode           (juce::Graphics& g, const Node& node);
    void drawNode           (juce::Graphics& g, const Node& node, juce::Rectangle<float> circleBoundsOverride);
    void drawRootNode       (juce::Graphics& g, const RootNode& node);
    void drawRootNodeRectangle(juce::Graphics& g, const RootRectangle& rootRectangle);

    void drawNodeArrowText          (juce::Graphics &g, const NodeArrow &nodeArrow, const juce::TextEditor &editor, TextCords textCords);

    void drawNodeArrow      (juce::Graphics& g, const NodeArrow& nodeArrow, const juce::TextEditor& editor);
    void drawDanglingArrow  (juce::Graphics& g, const DanglingArrow& danglingArrow);

    void drawPlayButton     (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const PlayButton& button);
    void drawSyncButton     (juce::Graphics& g, bool isMouseOver, bool isButtonDown, const SyncButton& button);

    void drawNodeButton     (juce::Graphics& g, const NodeButton& nodeButton);
    void drawModulatorButton(juce::Graphics& g, const ModulatorButton& modulatorButton);
    void drawTraversalFlagButton(juce::Graphics& g, const TraversalFlagButton& traversalFlagButton);



    void drawUndoButton     (juce::Graphics& g, const UndoButton& undoButton, bool isButtonDown);
    void drawRedoButton     (juce::Graphics& g, const RedoButton& redoButton, bool isButtonDown);
    void drawUndoRedoPane   (juce::Graphics& g, const UndoRedoPane& undoRedoPane);
    void drawResetButton    (juce::Graphics& g, const ResetButton& resetButton, bool isButtonDown);

    void drawPaintTool         (juce::Graphics& g, const PaintTool& paintTool);
    void drawPaintToolSettings (juce::Graphics& g, const PaintToolSettings& paintToolSettings);
    void drawArrowTool         (juce::Graphics& g, const ArrowTool& arrowTool);

};

#endif //SEQUENCETREE_CUSTOMLOOKANDFEEL_H