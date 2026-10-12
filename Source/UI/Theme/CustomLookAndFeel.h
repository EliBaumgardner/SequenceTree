#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Theme.h"
#include "Visuals.h"

class FileLabel;

class ValueSlider;

struct ButtonState;

class CustomLookAndFeel : public juce::LookAndFeel_V4, public Theme
{
public:

    CustomLookAndFeel();

    void applyThemeColour(juce::Colour themeColour);

    static CustomLookAndFeel& get(const juce::Component& component)
    {
        return static_cast<CustomLookAndFeel&>(component.getLookAndFeel());
    }

    void drawPopupMenuBackgroundWithOptions(juce::Graphics& graphics, int width, int height, const juce::PopupMenu::Options& options) override;
    void getIdealPopupMenuItemSize         (const juce::String& text, bool isSeparator, int standardMenuItemHeight, int& idealWidth, int& idealHeight) override;

    void drawPopupMenuItem(juce::Graphics& graphics, const juce::Rectangle<int>& area, bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                           bool hasSubMenu, const juce::String& text, const juce::String& shortcutKeyText, const juce::Drawable* icon,
                           const juce::Colour* textColourToUse) override;

    void drawScrollbar(juce::Graphics& graphics, juce::ScrollBar& scrollBar, int x, int y, int width, int height, bool isScrollbarVertical,
                       int thumbStartPosition, int thumbSize, bool isMouseOver, bool isMouseDown) override;

    void drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& graphics, const juce::Path& path, juce::Image& cachedShadow) override;

    void drawCanvas      (juce::Graphics& graphics, const CanvasGrid& grid);
    void drawPane        (juce::Graphics& graphics, juce::Rectangle<float> bounds);
    void drawFrostedGlass(juce::Graphics& graphics, juce::Component& surface, juce::Component& backdrop, juce::Rectangle<int> area);
    void blurFrostedImage(juce::Image& frosted);

    void drawCircleNode (juce::Graphics& graphics, const NodeVisual& visual);
    void drawSquareNode (juce::Graphics& graphics, const NodeVisual& visual);
    void drawRaisedPanel(juce::Graphics& graphics, juce::Rectangle<float> bounds);
    void drawArrow      (juce::Graphics& graphics, const ArrowVisual& visual);

    void drawForkIcon    (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRingDotIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRouteIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSettingsIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawPlayIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawLinkIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawMetronomeIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawSkipBackIcon (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawReloadIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawUndoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRedoIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawCircleIcon       (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRoundedSquareIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawTriangleIcon     (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawChevronDownIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawChevronUpIcon  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawEyeIcon        (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawAddIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawRemoveIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawImportIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawBrushIcon    (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawDashedBoxIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawQuaverIcon   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawCircledHButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawArrowButton   (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawPencilButton  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawUploadButton  (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawWaveButton    (juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawPencilIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawEraserIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    void drawStrokeSymbol(juce::Graphics& graphics, juce::Rectangle<float> bounds, const std::vector<SymbolStroke>& strokes);
    void drawSymbolMark  (juce::Graphics& graphics, juce::Rectangle<float> bounds, SymbolMark mark, const std::vector<SymbolStroke>& strokes);

    void drawTextButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);
    void drawCodeButton(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state, float fontHeight = labelFontHeight);

    void drawIncrementIcon(juce::Graphics& graphics, juce::Rectangle<float> bounds, bool pointsUp);

    void drawValueSlider(juce::Graphics& graphics, const ValueSlider& slider);
    void drawFileLabel  (juce::Graphics& graphics, const FileLabel& fileLabel);

    int getPopupMenuBorderSize  () override;
    int getDefaultScrollbarWidth() override;

    juce::Font getPopupMenuFont() override;

    juce::CaretComponent* createCaretComponent(juce::Component* keyFocusOwner) override;

    juce::Rectangle<float> drawButtonTile(juce::Graphics& graphics, juce::Rectangle<float> bounds, const ButtonState& state);

    juce::Colour pressableButtonColour(const ButtonState& state) const;

    static juce::Rectangle<float> getNodeCircleBounds(juce::Rectangle<float> componentBounds);

    juce::Rectangle<int> unfrostedArea;
    juce::MouseCursor    pencilCursor;
    juce::MouseCursor    eraserCursor;
    bool                 frostedBackdropStale = true;

private:

    struct GlyphPaths
    {
        juce::Path add;
        juce::Path arrow;
        juce::Path boxDash;
        juce::Path boxDots;
        juce::Path brushHandle;
        juce::Path brushTip;
        juce::Path chevronDown;
        juce::Path chevronUp;
        juce::Path circle;
        juce::Path circledH;
        juce::Path code;
        juce::Path dashedBox;
        juce::Path dot;
        juce::Path eraser;
        juce::Path eye;
        juce::Path fork;
        juce::Path import;
        juce::Path link;
        juce::Path metronome;
        juce::Path pause;
        juce::Path pencil;
        juce::Path play;
        juce::Path quaverHead;
        juce::Path quaverStem;
        juce::Path redo;
        juce::Path reload;
        juce::Path remove;
        juce::Path ring;
        juce::Path roundedSquare;
        juce::Path route;
        juce::Path settings;
        juce::Path skipBar;
        juce::Path skipTriangle;
        juce::Path triangle;
        juce::Path undo;
        juce::Path upload;
        juce::Path wave;
    };

    juce::Image frostedBackdrop;
    GlyphPaths  glyphs;
};
