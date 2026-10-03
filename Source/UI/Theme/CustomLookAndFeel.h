#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"

class NodeCanvas;

class Arrow;

class FileLabel;

struct ButtonState;
struct NodeVisual;

class CustomLookAndFeel : public juce::LookAndFeel_V4, public Theme
{
public:

    CustomLookAndFeel();

    static CustomLookAndFeel& get(juce::Component& component)
    {
        return static_cast<CustomLookAndFeel&>(component.getLookAndFeel());
    }

    void drawPopupMenuBackgroundWithOptions (juce::Graphics& graphics, int width, int height,
                                             const juce::PopupMenu::Options& options) override;

    void drawPopupMenuItem (juce::Graphics& graphics, const juce::Rectangle<int>& area,
                            bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                            bool hasSubMenu, const juce::String& text,
                            const juce::String& shortcutKeyText,
                            const juce::Drawable* icon, const juce::Colour* textColourToUse) override;

    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                    int standardMenuItemHeight,
                                    int& idealWidth, int& idealHeight) override;

    juce::Font getPopupMenuFont() override;
    int getPopupMenuBorderSize() override;

    void drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& graphics, const juce::Path& path,
                                  juce::Image& cachedShadow) override;

    int  getDefaultScrollbarWidth() override;
    void drawScrollbar (juce::Graphics& graphics, juce::ScrollBar& scrollBar, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;

    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;

    void drawCanvas         (juce::Graphics& graphics, const NodeCanvas& canvas);

    void drawNodeIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTreeIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    juce::Colour pressableButtonColour(const ButtonState& state) const;

    static juce::Rectangle<float> getNodeCircleBounds(juce::Rectangle<float> componentBounds);

    void drawNode          (juce::Graphics& graphics, const NodeVisual& visual);
    void drawModulatorNode (juce::Graphics& graphics, const NodeVisual& visual);
    void drawRootRectangle (juce::Graphics& graphics, juce::Rectangle<float> bounds);

    void drawArrow          (juce::Graphics& graphics, const Arrow& arrow);

    void drawPlayIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawNodeModeIcon      (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawModulatorIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalFlagIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawDisplayArrowIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawIncrementIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, bool pointsUp);

    void drawTextButton        (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state,
                                float fontHeight = labelFontHeight);

    void drawAddIcon        (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRemoveIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawUndoIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRedoIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawResetIcon      (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSyncIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawPaintToolIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawArrowToolIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSpanToolIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawQuaverToolIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawNodeArrowIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPolyphonicArrowIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalArrowIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawFileLabel(juce::Graphics& graphics, const FileLabel& fileLabel);
};
