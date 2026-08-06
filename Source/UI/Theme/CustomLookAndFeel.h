//
// Created by Eli Baumgardner on 11/4/25.
//

#ifndef SEQUENCETREE_CUSTOMLOOKANDFEEL_H
#define SEQUENCETREE_CUSTOMLOOKANDFEEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

class NodeCanvas;

class Arrow;
struct ArrowGeometry;

class CustomTextEditor;

class PaintToolSettings;

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

    void drawPopupMenuBackgroundWithOptions (juce::Graphics& g, int width, int height,
                                             const juce::PopupMenu::Options& options) override;

    void drawPopupMenuItem (juce::Graphics& g, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                            bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColourToUse) override;

    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                    int standardMenuItemHeight,
                                    int& idealWidth, int& idealHeight) override;

    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override;

    void drawEditor         (juce::Graphics& g, CustomTextEditor& editor);
    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;

    void drawCanvas         (juce::Graphics& g, const NodeCanvas& canvas);
    void drawBar            (juce::Graphics& g, juce::Rectangle<float> bounds, bool isLitFromTop);


    void drawNodeIcon       (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTreeIcon       (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalIcon  (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);



    juce::Colour pressableButtonColour(const ButtonState& state) const;
    juce::Colour selectableButtonColour(const ButtonState& state) const;

    static juce::Rectangle<float> getNodeCircleBounds(juce::Rectangle<float> componentBounds);

    void drawNode          (juce::Graphics& g, const NodeVisual& visual);
    void drawModulatorNode (juce::Graphics& g, const NodeVisual& visual);
    void drawRootRectangle (juce::Graphics& g, juce::Rectangle<float> bounds);

    void drawArrow          (juce::Graphics& g, const Arrow& arrow);

    void drawPlayIcon       (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSyncIcon       (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawNodeModeIcon      (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawModulatorIcon     (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalFlagIcon (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawDisplayArrowIcon  (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawIncrementIcon     (juce::Graphics& g, juce::Rectangle<float> bounds, bool pointsUp);

    void drawTextButton        (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state,
                                float fontHeight = labelFontHeight);

    void drawUndoIcon       (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRedoIcon       (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawResetIcon      (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawPaintToolIcon  (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawArrowToolIcon  (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawNodeArrowIcon  (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPolyphonicArrowIcon (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalArrowIcon  (juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPaintToolSettings (juce::Graphics& g, const PaintToolSettings& paintToolSettings);

};

#endif //SEQUENCETREE_CUSTOMLOOKANDFEEL_H