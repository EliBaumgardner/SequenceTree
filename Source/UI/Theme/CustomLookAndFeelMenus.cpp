#include "CustomLookAndFeel.h"
#include "BinaryData.h"
#include "IconData.h"

#include <juce_gui_extra/juce_gui_extra.h>

static juce::Path loadGlyph(const char* svgData, int svgSize)
{
    const std::unique_ptr<juce::Drawable> svg = juce::Drawable::createFromImageData(svgData, static_cast<size_t>(svgSize));
    juce::Path                            glyph;

    for (juce::Component* shape : svg->getChildren()) {
        glyph.addPath(static_cast<juce::DrawablePath*>(shape)->getPath());
    }

    return glyph;
}

CustomLookAndFeel::CustomLookAndFeel()
{
    const int                  cursorPixels = static_cast<int>(glyphGridSize * cursorScale);
    const juce::PathStrokeType cursorInk(glyphGridSize * glyphStrokeRatio, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    const juce::PathStrokeType cursorHalo(cursorHaloWidth, juce::PathStrokeType::curved, juce::PathStrokeType::rounded);
    juce::Image                pencilImage(juce::Image::ARGB, cursorPixels, cursorPixels, true, juce::SoftwareImageType());
    juce::Image                eraserImage(juce::Image::ARGB, cursorPixels, cursorPixels, true, juce::SoftwareImageType());
    juce::Graphics             pencilGraphics(pencilImage);
    juce::Graphics             eraserGraphics(eraserImage);

    regularTypeface  = juce::Typeface::createSystemTypefaceFor(BinaryData::GeistRegular_ttf, BinaryData::GeistRegular_ttfSize);
    semiBoldTypeface = juce::Typeface::createSystemTypefaceFor(BinaryData::GeistSemiBold_ttf, BinaryData::GeistSemiBold_ttfSize);
    monoTypeface     = juce::Typeface::createSystemTypefaceFor(BinaryData::GeistMonoRegular_ttf, BinaryData::GeistMonoRegular_ttfSize);

    glyphs.add           = loadGlyph(IconData::Add_svg, IconData::Add_svgSize);
    glyphs.chevronDown   = loadGlyph(IconData::ChevronDown_svg, IconData::ChevronDown_svgSize);
    glyphs.chevronUp     = loadGlyph(IconData::ChevronUp_svg, IconData::ChevronUp_svgSize);
    glyphs.danglingArrow = loadGlyph(IconData::DanglingArrow_svg, IconData::DanglingArrow_svgSize);
    glyphs.eraser        = loadGlyph(IconData::Eraser_svg, IconData::Eraser_svgSize);
    glyphs.eye           = loadGlyph(IconData::Eye_svg, IconData::Eye_svgSize);
    glyphs.host          = loadGlyph(IconData::Host_svg, IconData::Host_svgSize);
    glyphs.import        = loadGlyph(IconData::Import_svg, IconData::Import_svgSize);
    glyphs.modulator     = loadGlyph(IconData::Modulator_svg, IconData::Modulator_svgSize);
    glyphs.nodeDot       = loadGlyph(IconData::NodeDot_svg, IconData::NodeDot_svgSize);
    glyphs.nodeMode      = loadGlyph(IconData::NodeMode_svg, IconData::NodeMode_svgSize);
    glyphs.nodeRing      = loadGlyph(IconData::NodeRing_svg, IconData::NodeRing_svgSize);
    glyphs.ornament      = loadGlyph(IconData::Ornament_svg, IconData::Ornament_svgSize);
    glyphs.paintHandle   = loadGlyph(IconData::PaintHandle_svg, IconData::PaintHandle_svgSize);
    glyphs.paintTip      = loadGlyph(IconData::PaintTip_svg, IconData::PaintTip_svgSize);
    glyphs.pause         = loadGlyph(IconData::Pause_svg, IconData::Pause_svgSize);
    glyphs.pencil        = loadGlyph(IconData::Pencil_svg, IconData::Pencil_svgSize);
    glyphs.play          = loadGlyph(IconData::Play_svg, IconData::Play_svgSize);
    glyphs.publish       = loadGlyph(IconData::Publish_svg, IconData::Publish_svgSize);
    glyphs.quaverHead    = loadGlyph(IconData::QuaverHead_svg, IconData::QuaverHead_svgSize);
    glyphs.quaverStem    = loadGlyph(IconData::QuaverStem_svg, IconData::QuaverStem_svgSize);
    glyphs.redo          = loadGlyph(IconData::Redo_svg, IconData::Redo_svgSize);
    glyphs.remove        = loadGlyph(IconData::Remove_svg, IconData::Remove_svgSize);
    glyphs.resetBar      = loadGlyph(IconData::ResetBar_svg, IconData::ResetBar_svgSize);
    glyphs.resetPatch    = loadGlyph(IconData::ResetPatch_svg, IconData::ResetPatch_svgSize);
    glyphs.resetTriangle = loadGlyph(IconData::ResetTriangle_svg, IconData::ResetTriangle_svgSize);
    glyphs.rules         = loadGlyph(IconData::Rules_svg, IconData::Rules_svgSize);
    glyphs.settings      = loadGlyph(IconData::Settings_svg, IconData::Settings_svgSize);
    glyphs.spanBridge    = loadGlyph(IconData::SpanBridge_svg, IconData::SpanBridge_svgSize);
    glyphs.spanEnds      = loadGlyph(IconData::SpanEnds_svg, IconData::SpanEnds_svgSize);
    glyphs.spanOutline   = loadGlyph(IconData::SpanOutline_svg, IconData::SpanOutline_svgSize);
    glyphs.sync          = loadGlyph(IconData::Sync_svg, IconData::Sync_svgSize);
    glyphs.tempo         = loadGlyph(IconData::Tempo_svg, IconData::Tempo_svgSize);
    glyphs.traversal     = loadGlyph(IconData::Traversal_svg, IconData::Traversal_svgSize);
    glyphs.traversalFlag = loadGlyph(IconData::TraversalFlag_svg, IconData::TraversalFlag_svgSize);
    glyphs.tree          = loadGlyph(IconData::Tree_svg, IconData::Tree_svgSize);
    glyphs.undo          = loadGlyph(IconData::Undo_svg, IconData::Undo_svgSize);

    pencilGraphics.addTransform(juce::AffineTransform::scale(cursorScale));
    pencilGraphics.setColour(windowColour);
    pencilGraphics.strokePath(glyphs.pencil, cursorHalo);
    pencilGraphics.setColour(textColour);
    pencilGraphics.strokePath(glyphs.pencil, cursorInk);

    eraserGraphics.addTransform(juce::AffineTransform::scale(cursorScale));
    eraserGraphics.setColour(windowColour);
    eraserGraphics.strokePath(glyphs.eraser, cursorHalo);
    eraserGraphics.setColour(textColour);
    eraserGraphics.strokePath(glyphs.eraser, cursorInk);

    pencilCursor = juce::MouseCursor(pencilImage, pencilHotSpotX, pencilHotSpotY, cursorScale);
    eraserCursor = juce::MouseCursor(eraserImage, eraserHotSpotX, eraserHotSpotY, cursorScale);

    applyThemeColour(accentColour);
}

void CustomLookAndFeel::applyThemeColour(juce::Colour themeColour)
{
    recolour(themeColour);

    setColour(juce::PopupMenu::backgroundColourId,            juce::Colours::transparentBlack);
    setColour(juce::PopupMenu::textColourId,                  popupMenuTextColour);
    setColour(juce::PopupMenu::headerTextColourId,            popupMenuTextColour);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, popupMenuHighlightColour);
    setColour(juce::PopupMenu::highlightedTextColourId,       popupMenuHighlightTextColour);
    setColour(juce::CaretComponent::caretColourId,            accentColour);
    setColour(juce::TextEditor::highlightColourId,            accentSoftColour);
    setColour(juce::TextEditor::highlightedTextColourId,      textColour);
    setColour(juce::TooltipWindow::backgroundColourId,        raisedColour);
    setColour(juce::TooltipWindow::textColourId,              textColour);
    setColour(juce::TooltipWindow::outlineColourId,           juce::Colours::transparentBlack);
    setColour(juce::Label::textColourId,                      captionColour);
    setColour(juce::TextEditor::textColourId,                 textColour);
    setColour(juce::CodeEditorComponent::backgroundColourId,  surfaceColour);
    setColour(juce::CodeEditorComponent::defaultTextColourId, textColour);
    setColour(juce::CodeEditorComponent::highlightColourId,   accentColour.withAlpha(codeSelectionAlpha));
    setColour(juce::CodeEditorComponent::lineNumberTextId,    lineNumberColour);
}

void CustomLookAndFeel::drawPopupMenuBackgroundWithOptions(juce::Graphics& graphics, int width, int height,
                                                           const juce::PopupMenu::Options&)
{
    const auto bounds = juce::Rectangle<float>(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height));

    graphics.setColour(popupMenuColour);
    graphics.fillRoundedRectangle(bounds, paneCornerRadius);
}

void CustomLookAndFeel::getIdealPopupMenuItemSize(const juce::String& text, bool isSeparator,
                                                  int standardMenuItemHeight,
                                                  int& idealWidth, int& idealHeight)
{
    int itemHeight = popupMenuItemHeight;

    if (standardMenuItemHeight > 0) {
        itemHeight = standardMenuItemHeight;
    }

    if (isSeparator) {
        idealWidth  = 50;
        idealHeight = itemHeight / 2;
        return;
    }

    idealHeight = itemHeight;
    idealWidth  = juce::GlyphArrangement::getStringWidthInt(getPopupMenuFont(), text)
                    + idealHeight
                    + static_cast<int>(popupMenuTextInset * 4.0f);
}

void CustomLookAndFeel::drawPopupMenuItem(juce::Graphics& graphics, const juce::Rectangle<int>& area,
                                          bool isSeparator, bool isActive, bool isHighlighted, bool isTicked,
                                          bool hasSubMenu, const juce::String& text,
                                          const juce::String& shortcutKeyText,
                                          const juce::Drawable* icon, const juce::Colour* textColourToUse)
{
    if (isSeparator) {
        auto line = area.toFloat().reduced(menuEdgeInset, 0.0f).withHeight(1.0f);
        line.setY(area.toFloat().getCentreY());

        graphics.setColour(popupMenuTextColour.withAlpha(0.25f));
        graphics.fillRect(line);
        return;
    }

    const auto itemBounds = area.toFloat().reduced(popupMenuItemInset, popupMenuItemGap);

    juce::Colour itemTextColour = popupMenuTextColour;

    if (textColourToUse != nullptr) {
        itemTextColour = *textColourToUse;
    }

    if (isHighlighted && isActive) {
        graphics.setColour(popupMenuHighlightColour);
        graphics.fillRoundedRectangle(itemBounds, paneCornerRadius);

        itemTextColour = popupMenuHighlightTextColour;
    }

    float textAlpha = 0.4f;

    if (isActive) {
        textAlpha = 1.0f;
    }

    graphics.setColour(itemTextColour.withMultipliedAlpha(textAlpha));
    graphics.setFont(getPopupMenuFont());

    auto textBounds = itemBounds.reduced(popupMenuTextInset, 0.0f);

    const auto gutter = textBounds.removeFromLeft(textBounds.getHeight());

    if (icon != nullptr) {
        icon->drawWithin(graphics, gutter, juce::RectanglePlacement::centred | juce::RectanglePlacement::onlyReduceInSize, 1.0f);
    }
    else if (isTicked) {
        const juce::Path tick = getTickShape(1.0f);
        graphics.fillPath(tick, tick.getTransformToScaleToFit(gutter.reduced(gutter.getWidth() / 4.0f), true));
    }

    if (hasSubMenu) {
        const float arrowHeight = 0.6f * getPopupMenuFont().getAscent();
        const float arrowX      = textBounds.removeFromRight(arrowHeight).getX();
        const float centreY     = textBounds.getCentreY();
        juce::Path  arrow;

        arrow.startNewSubPath(arrowX, centreY - arrowHeight * 0.5f);
        arrow.lineTo(arrowX + arrowHeight * 0.6f, centreY);
        arrow.lineTo(arrowX, centreY + arrowHeight * 0.5f);

        graphics.strokePath(arrow, juce::PathStrokeType(1.5f));
    }

    graphics.drawFittedText(text, textBounds.toNearestInt(), juce::Justification::centredLeft, 1);

    if (shortcutKeyText.isNotEmpty()) {
        graphics.setFont(font(FontStyle::Mono, labelFontHeight * 0.85f));
        graphics.drawText(shortcutKeyText, textBounds, juce::Justification::centredRight, true);
    }
}

void CustomLookAndFeel::drawCallOutBoxBackground(juce::CallOutBox& box, juce::Graphics& graphics, const juce::Path& path,
                                                 juce::Image& cachedShadow)
{
    if (cachedShadow.isNull()) {
        cachedShadow = juce::Image(juce::Image::ARGB, box.getWidth(), box.getHeight(), true);

        juce::Graphics shadowGraphics(cachedShadow);

        juce::DropShadow(dropShadowColour.withAlpha(callOutShadowAlpha), callOutShadowRadius, {}).drawForPath(shadowGraphics, path);
    }

    graphics.setColour(dropShadowColour);
    graphics.drawImageAt(cachedShadow, 0, 0);

    graphics.setColour(popupMenuColour);
    graphics.fillPath(path);

    graphics.setColour(popupMenuBorderColour);
    graphics.strokePath(path, juce::PathStrokeType(popupMenuBorderThickness));
}

int CustomLookAndFeel::getPopupMenuBorderSize()
{
    return popupMenuPadding;
}

juce::Font CustomLookAndFeel::getPopupMenuFont()
{
    return font(FontStyle::Regular, labelFontHeight);
}
