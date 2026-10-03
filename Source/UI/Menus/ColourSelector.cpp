#include "ColourSelector.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Plugin/PluginProcessor.h"
#include "../Theme/CustomLookAndFeel.h"

ColourPicker::ColourPicker(const ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(context.lookAndFeel);
}

ColourPicker::~ColourPicker()
{
    setLookAndFeel(nullptr);
}

void ColourPicker::paint(juce::Graphics& graphics)
{
    const Theme&             theme                      = CustomLookAndFeel::get(*this);
    const juce::Colour       currentColour              = juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
    const juce::ValueTree    presets                    = applicationContext.processor->colourPresets;
    const float              cursorDiameter             = getWidth() * cursorRadiusRatio * 2.0f;
    const float              presetGap                  = getWidth() * presetGapRatio;
    const float              presetWidth                = (presetArea.getWidth() - presetGap * (presetCount - 1)) / presetCount;
    const juce::Point<float> saturationBrightnessCentre { saturationBrightnessArea.getX() + saturation * saturationBrightnessArea.getWidth(),
                                                          saturationBrightnessArea.getY() + (1.0f - brightness) * saturationBrightnessArea.getHeight() };
    const juce::Point<float> hueCentre                  { hueArea.getX() + hue * hueArea.getWidth(), static_cast<float>(hueArea.getCentreY()) };
    const auto               saturationBrightnessCursor = juce::Rectangle<float>(cursorDiameter, cursorDiameter).withCentre(saturationBrightnessCentre);
    const auto               hueCursor                  = juce::Rectangle<float>(cursorDiameter, cursorDiameter).withCentre(hueCentre);

    graphics.drawImage(saturationBrightnessImage, saturationBrightnessArea.toFloat());
    graphics.drawImage(hueImage, hueArea.toFloat());

    graphics.setColour(theme.popupMenuBorderColour);
    graphics.drawRect(saturationBrightnessArea);
    graphics.drawRect(hueArea);

    graphics.setColour(theme.selectionRingColour);
    graphics.drawEllipse(saturationBrightnessCursor.expanded(cursorRingWidth), cursorRingWidth);
    graphics.drawEllipse(hueCursor.expanded(cursorRingWidth), cursorRingWidth);

    graphics.setColour(theme.hoverRingColour);
    graphics.drawEllipse(saturationBrightnessCursor, cursorRingWidth);
    graphics.drawEllipse(hueCursor, cursorRingWidth);

    graphics.setColour(originalColour);
    graphics.fillRect(originalArea);

    graphics.setColour(currentColour);
    graphics.fillRect(currentArea);

    graphics.setColour(currentColour.contrasting());
    graphics.setFont(juce::FontOptions(currentArea.getHeight() * hexTextHeightRatio));
    graphics.drawText("#" + currentColour.toDisplayString(false), currentArea, juce::Justification::centred);

    graphics.setColour(theme.popupMenuBorderColour);
    graphics.drawRect(originalArea.getUnion(currentArea));

    for (int presetIndex = 0; presetIndex < presetCount; ++presetIndex) {
        const juce::ValueTree preset      = presets.getChild(presetIndex);
        const bool            isSet       = preset.hasProperty(ValueTreeIdentifiers::PresetColour);
        const auto            slot        = juce::Rectangle<float>(presetArea.getX() + presetIndex * (presetWidth + presetGap),
                                                                   static_cast<float>(presetArea.getY()), presetWidth,
                                                                   static_cast<float>(presetArea.getHeight()));
        const float           glyphLength = slot.getWidth() * presetGlyphRatio;

        if (isSet) {
            graphics.setColour(juce::Colour::fromString(preset.getProperty(ValueTreeIdentifiers::PresetColour).toString()));
            graphics.fillRoundedRectangle(slot, Theme::paneCornerRadius);
        }
        else {
            graphics.setColour(theme.baseDarkColour2);
            graphics.fillRoundedRectangle(slot, Theme::paneCornerRadius);

            graphics.setColour(theme.captionColour);
            graphics.drawLine(slot.getCentreX() - glyphLength * 0.5f, slot.getCentreY(), slot.getCentreX() + glyphLength * 0.5f, slot.getCentreY());
            graphics.drawLine(slot.getCentreX(), slot.getCentreY() - glyphLength * 0.5f, slot.getCentreX(), slot.getCentreY() + glyphLength * 0.5f);
        }

        graphics.setColour(theme.popupMenuBorderColour);
        graphics.drawRoundedRectangle(slot, Theme::paneCornerRadius, Theme::popupMenuBorderThickness);
    }
}

void ColourPicker::resized()
{
    const int padding = juce::roundToInt(getWidth() * paddingRatio);
    const int gap     = juce::roundToInt(getWidth() * sectionGapRatio);
    auto      bounds  = getLocalBounds().reduced(padding);
    const int width   = bounds.getWidth();

    presetArea = bounds.removeFromBottom(juce::roundToInt(width * presetRowHeightRatio));
    bounds.removeFromBottom(gap);

    currentArea  = bounds.removeFromBottom(juce::roundToInt(width * swatchRowHeightRatio));
    originalArea = currentArea.removeFromLeft(currentArea.getWidth() / 2);
    bounds.removeFromBottom(gap);

    hueArea = bounds.removeFromBottom(juce::roundToInt(width * hueStripHeightRatio));
    bounds.removeFromBottom(gap);

    saturationBrightnessArea = bounds;
    hueImage                 = juce::Image(juce::Image::RGB, juce::jmax(1, hueArea.getWidth()), 1, false);

    for (int column = 0; column < hueImage.getWidth(); ++column) {
        hueImage.setPixelAt(column, 0, juce::Colour::fromHSV(static_cast<float>(column) / hueImage.getWidth(), 1.0f, 1.0f, 1.0f));
    }

    renderSaturationBrightnessImage();
}

void ColourPicker::mouseDown(const juce::MouseEvent& event)
{
    const juce::Point<int> position      = event.getPosition();
    const juce::Colour     currentColour = juce::Colour::fromHSV(hue, saturation, brightness, 1.0f);
    juce::ValueTree        presets       = applicationContext.processor->colourPresets;
    juce::Colour           chosenColour  = originalColour;

    applicationContext.undoManager->beginNewTransaction();

    if (saturationBrightnessArea.contains(position)) {
        dragTarget = DragTarget::SaturationBrightness;

        mouseDrag(event);

        return;
    }

    if (hueArea.contains(position)) {
        dragTarget = DragTarget::Hue;

        mouseDrag(event);

        return;
    }

    if (presetArea.contains(position)) {
        const int       presetIndex = juce::jlimit(0, presetCount - 1, (position.x - presetArea.getX()) * presetCount / presetArea.getWidth());
        juce::ValueTree preset      = presets.getChild(presetIndex);

        if (event.mods.isPopupMenu() || ! preset.hasProperty(ValueTreeIdentifiers::PresetColour)) {
            preset.setProperty(ValueTreeIdentifiers::PresetColour, currentColour.toString(), nullptr);

            repaint();

            return;
        }

        chosenColour = juce::Colour::fromString(preset.getProperty(ValueTreeIdentifiers::PresetColour).toString());
    }
    else if (! originalArea.contains(position)) {
        return;
    }

    chosenColour.getHSB(hue, saturation, brightness);

    renderSaturationBrightnessImage();
    repaint();

    if (onColourPicked) {
        onColourPicked(chosenColour);
    }
}

void ColourPicker::mouseDrag(const juce::MouseEvent& event)
{
    const juce::Point<int> position           = event.getPosition();
    const float            horizontalFraction = static_cast<float>(position.x - saturationBrightnessArea.getX()) / saturationBrightnessArea.getWidth();
    const float            verticalFraction   = static_cast<float>(position.y - saturationBrightnessArea.getY()) / saturationBrightnessArea.getHeight();
    const float            hueFraction        = static_cast<float>(position.x - hueArea.getX()) / hueArea.getWidth();

    switch (dragTarget) {
        case DragTarget::SaturationBrightness:
            saturation = juce::jlimit(0.0f, 1.0f, horizontalFraction);
            brightness = 1.0f - juce::jlimit(0.0f, 1.0f, verticalFraction);
            break;

        case DragTarget::Hue:
            hue = juce::jlimit(0.0f, 1.0f, hueFraction);

            renderSaturationBrightnessImage();
            break;

        case DragTarget::None:
            return;
    }

    repaint();

    if (onColourPicked) {
        onColourPicked(juce::Colour::fromHSV(hue, saturation, brightness, 1.0f));
    }
}

void ColourPicker::mouseUp(const juce::MouseEvent&)
{
    dragTarget = DragTarget::None;
}

void ColourPicker::showColour(juce::Colour colour)
{
    juce::ValueTree presets = applicationContext.processor->colourPresets;

    originalColour = colour;

    colour.getHSB(hue, saturation, brightness);

    while (presets.getNumChildren() < presetCount) {
        presets.appendChild(juce::ValueTree(ValueTreeIdentifiers::ColourPreset), nullptr);
    }

    renderSaturationBrightnessImage();
    repaint();
}

void ColourPicker::renderSaturationBrightnessImage()
{
    const int width  = juce::jmax(1, saturationBrightnessArea.getWidth());
    const int height = juce::jmax(1, saturationBrightnessArea.getHeight());

    saturationBrightnessImage = juce::Image(juce::Image::RGB, width, height, false);

    juce::Image::BitmapData pixels(saturationBrightnessImage, juce::Image::BitmapData::writeOnly);

    for (int row = 0; row < height; ++row) {
        for (int column = 0; column < width; ++column) {
            const float pixelSaturation = static_cast<float>(column) / width;
            const float pixelBrightness = 1.0f - static_cast<float>(row) / height;

            pixels.setPixelColour(column, row, juce::Colour::fromHSV(hue, pixelSaturation, pixelBrightness, 1.0f));
        }
    }
}

ColourSelector::ColourSelector(const ApplicationContext& context)
    : picker(context)
{
    setRepaintsOnMouseActivity(true);

    picker.onColourPicked = [this](juce::Colour pickedColour) {
        colour = pickedColour;

        repaint();

        if (onColourPicked) {
            onColourPicked(pickedColour);
        }
    };
}

void ColourSelector::paint(juce::Graphics& graphics)
{
    const Theme& theme   = CustomLookAndFeel::get(*this);
    const auto   bounds  = getLocalBounds().toFloat().reduced(Theme::popupMenuBorderThickness);
    juce::Colour fill    = colour;
    juce::Colour outline = theme.popupMenuBorderColour;

    if (! isEnabled()) {
        fill = theme.buttonBarColour;
    }

    if (isEnabled() && isMouseOver()) {
        outline = theme.hoverRingColour;
    }

    switch (shape) {
        case Shape::Circle:
            graphics.setColour(fill);
            graphics.fillEllipse(bounds);

            graphics.setColour(outline);
            graphics.drawEllipse(bounds, Theme::popupMenuBorderThickness);
            break;

        case Shape::Square:
            graphics.setColour(fill);
            graphics.fillRoundedRectangle(bounds, Theme::paneCornerRadius);

            graphics.setColour(outline);
            graphics.drawRoundedRectangle(bounds, Theme::paneCornerRadius, Theme::popupMenuBorderThickness);
            break;
    }
}

void ColourSelector::mouseDown(const juce::MouseEvent&)
{
    juce::Component* const editor      = getTopLevelComponent();
    const int              pickerWidth = juce::roundToInt(editor->getHeight() * pickerWidthRatio);

    picker.setSize(pickerWidth, juce::roundToInt(pickerWidth * ColourPicker::heightToWidthRatio));

    picker.showColour(colour);

    pickerBox = std::make_unique<juce::CallOutBox>(picker, editor->getLocalArea(this, getLocalBounds()), editor);

    pickerBox->setLookAndFeel(&getLookAndFeel());

    pickerBox->enterModalState(true);
}
