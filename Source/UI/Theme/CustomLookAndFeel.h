#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "../../Util/NodeInfo.h"

class NodeCanvas;

class Arrow;

class FileLabel;

class ValueSlider;

struct ButtonState;
struct NodeVisual;

class CustomLookAndFeel : public juce::LookAndFeel_V4, public Theme
{
public:

    CustomLookAndFeel();

    void applyThemeColour(juce::Colour themeColour);

    static CustomLookAndFeel& get(const juce::Component& component)
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

    juce::Font getPopupMenuFont() override;

    void getIdealPopupMenuItemSize (const juce::String& text, bool isSeparator,
                                    int standardMenuItemHeight,
                                    int& idealWidth, int& idealHeight) override;

    int getPopupMenuBorderSize() override;

    void drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& graphics, const juce::Path& path,
                                  juce::Image& cachedShadow) override;

    int  getDefaultScrollbarWidth() override;

    void drawScrollbar (juce::Graphics& graphics, juce::ScrollBar& scrollBar, int x, int y, int width, int height,
                        bool isScrollbarVertical, int thumbStartPosition, int thumbSize,
                        bool isMouseOver, bool isMouseDown) override;

    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;
    void                          drawCanvas(juce::Graphics& graphics, const NodeCanvas& nodeCanvas);
    void drawPane           (juce::Graphics& graphics, juce::Rectangle<float> bounds);
    void                          drawFrostedGlass(juce::Graphics& graphics, juce::Component& surface, NodeCanvas& nodeCanvas, juce::Rectangle<int> area);
    void blurFrostedImage   (juce::Image& frosted);
    juce::Rectangle<float> drawButtonTile(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    juce::Colour pressableButtonColour(const ButtonState& state) const;
    void drawNodeIcon      (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTreeIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTraversalIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSettingsIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
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
    void drawCollapseIcon      (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawIncrementIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, bool pointsUp);
    void drawEyeIcon           (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTempoIcon         (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawRulesButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state,
                         float fontHeight = labelFontHeight);

    void drawHostButton         (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawDanglingArrowButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPencilButton       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPublishButton      (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawAddIcon        (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRemoveIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawImportIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawUndoIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRedoIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawResetIcon      (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawResetPatchIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSyncIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPaintToolIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSpanToolIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawQuaverToolIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawOrnamentButton (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPencilIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawEraserIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawOrnamentSymbol (juce::Graphics& graphics, juce::Rectangle<float> bounds, const juce::String& symbol);
    void drawNodeOrnament   (juce::Graphics& graphics, juce::Rectangle<float> bounds, OrnamentMark mark, const juce::String& symbol);
    void drawAxisButton       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawValueSlider      (juce::Graphics& graphics, const ValueSlider& slider);
    void drawFileLabel(juce::Graphics& graphics, const FileLabel& fileLabel);

    juce::Rectangle<int> unfrostedArea;
    juce::MouseCursor    pencilCursor;
    juce::MouseCursor    eraserCursor;
    bool                 frostedBackdropStale = true;

private:

    struct GlyphPaths
    {
        juce::Path add;
        juce::Path chevronDown;
        juce::Path chevronUp;
        juce::Path danglingArrow;
        juce::Path eraser;
        juce::Path eye;
        juce::Path host;
        juce::Path import;
        juce::Path modulator;
        juce::Path nodeDot;
        juce::Path nodeMode;
        juce::Path nodeRing;
        juce::Path ornament;
        juce::Path paintHandle;
        juce::Path paintTip;
        juce::Path pause;
        juce::Path pencil;
        juce::Path play;
        juce::Path publish;
        juce::Path quaverHead;
        juce::Path quaverStem;
        juce::Path redo;
        juce::Path remove;
        juce::Path resetBar;
        juce::Path resetPatch;
        juce::Path resetTriangle;
        juce::Path rules;
        juce::Path settings;
        juce::Path spanBridge;
        juce::Path spanEnds;
        juce::Path spanOutline;
        juce::Path sync;
        juce::Path tempo;
        juce::Path traversal;
        juce::Path traversalFlag;
        juce::Path tree;
        juce::Path undo;
    };

    juce::Image frostedBackdrop;
    GlyphPaths  glyphs;
};
