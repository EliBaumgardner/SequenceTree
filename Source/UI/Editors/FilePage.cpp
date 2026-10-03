#include "FilePage.h"

#include "../Theme/CustomLookAndFeel.h"

FilePage::FilePage(const ApplicationContext& context)
    : juce::CodeEditorComponent(*this, nullptr)
{
    const Theme& theme = *context.lookAndFeel;

    setNewLineCharacters("\n");
    addListener(this);

    setColour(backgroundColourId,               theme.surfaceColour);
    setColour(defaultTextColourId,              theme.textColour);
    setColour(highlightColourId,                theme.accentColour.withAlpha(selectionAlpha));
    setColour(lineNumberBackgroundId,           juce::Colours::transparentBlack);
    setColour(lineNumberTextId,                 theme.lineNumberColour);
    setColour(juce::CaretComponent::caretColourId, theme.accentColour);

    setLineNumbersShown(true);
    setTabSize(indentSize, true);
    setFont(theme.font(Theme::FontStyle::Mono, baseFontHeight));
}

FilePage::~FilePage()
{
    removeListener(this);
}

void FilePage::paintOverChildren(juce::Graphics& graphics)
{
    const int firstLine = getFirstLineOnScreen();
    const int lastLine  = firstLine + getNumLinesOnScreen();

    graphics.setColour(CustomLookAndFeel::get(*this).scriptErrorColour.withAlpha(errorLineAlpha));

    for (int line : errorLines) {
        if (line >= firstLine && line <= lastLine) {
            graphics.fillRect(0, (line - firstLine) * getLineHeight(), getWidth(), getLineHeight());
        }
    }
}

void FilePage::mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel)
{
    if (! event.mods.isShiftDown()) {
        juce::CodeEditorComponent::mouseWheelMove(event, wheel);
        return;
    }

    float delta = wheel.deltaY;

    if (wheel.isReversed) {
        delta = -delta;
    }

    setZoom(zoom * (1.0f + delta * zoomSensitivity));
}

void FilePage::setZoom(float newZoom)
{
    const float clamped = juce::jlimit(minZoom, maxZoom, newZoom);

    if (juce::approximatelyEqual(clamped, zoom)) {
        return;
    }

    zoom = clamped;

    setFont(getFont().withHeight(baseFontHeight * zoom));
}

void FilePage::mouseMagnify(const juce::MouseEvent&, float scaleFactor)
{
    setZoom(zoom * scaleFactor);
}

void FilePage::codeDocumentTextInserted(const juce::String&, int)
{
    if (! suppressTextChanged && onTextChanged != nullptr) {
        onTextChanged();
    }
}

void FilePage::codeDocumentTextDeleted(int, int)
{
    if (! suppressTextChanged && onTextChanged != nullptr) {
        onTextChanged();
    }
}
