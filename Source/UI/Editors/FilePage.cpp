//
// Created by Eli Baumgardner on 8/19/26.
//

#include "FilePage.h"

#include "LineEditor.h"
#include "../Theme/CustomLookAndFeel.h"

#include <cmath>


FilePage::FilePage(ApplicationContext &context) : context(context) {

    textMeasurer.setMultiLine(true, true);
    textMeasurer.setReturnKeyStartsNewLine(false);
    textMeasurer.setScrollbarsShown(false);
    textMeasurer.setBorder(juce::BorderSize<int>(0));
    textMeasurer.setIndents(0, 0);

    createNewFile();
}

int FilePage::rowCountFor(const juce::String& text, int editorWidth) {

    const juce::Font font { juce::FontOptions(fontHeight()) };

    const float textRowHeight = font.getHeight();

    if (editorWidth <= 0 || text.isEmpty() || textRowHeight <= 0.0f) {
        return 1;
    }

    textMeasurer.setFont(font);
    textMeasurer.setSize(editorWidth, 1);
    textMeasurer.setText(text, juce::dontSendNotification);

    return juce::jmax(1, juce::roundToInt((float) textMeasurer.getTextHeight() / textRowHeight));
}

void FilePage::paint(juce::Graphics &g) {

    g.setColour(CustomLookAndFeel::get(*this).baseDarkColour2);
    g.fillRect(getLocalBounds());
}

float FilePage::fontHeight() const {

    return baseFontHeight * zoom;
}

int FilePage::lineHeight() const {

    const juce::Font font { juce::FontOptions(fontHeight()) };

    return juce::jmax(1, (int) std::ceil(font.getHeight()));
}

int FilePage::preferredHeightForWidth(int width) {

    const int lineWidth = width - textAreaInset * 2;
    const int rowHeight = lineHeight();

    int total = textAreaInset * 2;

    for (const auto& fileLine : fileLines) {
        total += rowCountFor(fileLine->getText(), fileLine->editorWidthFor(lineWidth)) * rowHeight;
    }

    return total;
}

void FilePage::handleAsyncUpdate() {

    setSize(getWidth(), preferredHeightForWidth(getWidth()));
    resized();
}

void FilePage::mouseWheelMove(const juce::MouseEvent& e, const juce::MouseWheelDetails& wheel) {

    if (! e.mods.isShiftDown()) {
        juce::Component::mouseWheelMove(e, wheel);
        return;
    }

    const float delta = wheel.isReversed ? -wheel.deltaY : wheel.deltaY;

    setZoom(zoom * (1.0f + delta * zoomSensitivity));
}

void FilePage::mouseMagnify(const juce::MouseEvent&, float scaleFactor) {

    setZoom(zoom * scaleFactor);
}

void FilePage::setZoom(float newZoom) {

    const float clamped = juce::jlimit(minZoom, maxZoom, newZoom);

    if (juce::approximatelyEqual(clamped, zoom)) {
        return;
    }

    zoom = clamped;

    applyLineMetrics();
    setSize(getWidth(), preferredHeightForWidth(getWidth()));
    resized();
}

void FilePage::applyLineMetrics() {

    const float textHeight = fontHeight();

    const juce::Font gutterFont { juce::FontOptions(textHeight) };

    const int gutterWidth = juce::roundToInt(FileLine::gutterTextInset * 2 * zoom)
                          + (int) std::ceil(gutterFont.getStringWidthFloat(juce::String(fileLines.size())));

    const int rowHeight = lineHeight();

    for (auto& fileLine : fileLines) {
        fileLine->setGutterWidth(gutterWidth);
        fileLine->setFontHeight(textHeight);
        fileLine->setRowHeight(rowHeight);
    }
}

void FilePage::resized() {

    auto textArea = getLocalBounds().reduced(textAreaInset);

    const int rowHeight = lineHeight();
    const int lineWidth = textArea.getWidth();

    for (auto& fileLine : fileLines) {
        const int rows = rowCountFor(fileLine->getText(), fileLine->editorWidthFor(lineWidth));

        fileLine->setBounds(textArea.removeFromTop(rows * rowHeight));
    }
}

void FilePage::setFile(std::vector<std::unique_ptr<FileLine>> newFileLines) {

    fileLines = std::move(newFileLines);

    refreshLines();
    resized();
}

void FilePage::refreshLines() {

    for (int i = 0; i < (int) fileLines.size(); ++i) {

        FileLine& fileLine = *fileLines[i];

        fileLine.setLineNumber(i + 1);

        LineEditor& lineEditor = *fileLine.lineEditor;

        lineEditor.onInsertLine            = [this, line = &fileLine] { insertLineAfter(line); };
        lineEditor.onMoveToNextLine        = [this, line = &fileLine] { focusRelative(line, 1); };
        lineEditor.onMoveToPreviousLine    = [this, line = &fileLine] { focusRelative(line, -1); };
        lineEditor.onMergeWithPreviousLine = [this, line = &fileLine] { mergeWithPreviousLine(line); };
        lineEditor.onWrapChanged           = [this] {
            triggerAsyncUpdate();
            notifyTextChanged();
        };

        addAndMakeVisible(fileLine);
    }

    applyLineMetrics();
}

void FilePage::setText(const juce::String& text) {

    juce::StringArray lines;
    lines.addLines(text);

    if (lines.isEmpty()) {
        lines.add({});
    }

    std::vector<std::unique_ptr<FileLine>> newFileLines;
    newFileLines.reserve(lines.size());

    for (const juce::String& line : lines) {
        auto fileLine = std::make_unique<FileLine>(context);
        fileLine->lineEditor->setText(line);

        newFileLines.push_back(std::move(fileLine));
    }

    const juce::ScopedValueSetter<bool> silence(suppressTextChanged, true);

    setFile(std::move(newFileLines));
}

juce::String FilePage::getText() const {

    juce::StringArray lines;

    for (const auto& fileLine : fileLines) {
        lines.add(fileLine->getText());
    }

    return lines.joinIntoString("\n");
}

void FilePage::clearLineErrors() {

    for (auto& fileLine : fileLines) {
        fileLine->clearError();
    }
}

void FilePage::setLineError(int lineNumber, const juce::String& message) {

    const int index = lineNumber - 1;

    if (index >= 0 && index < (int) fileLines.size()) {
        fileLines[index]->setError(message);
    }
}

void FilePage::notifyTextChanged() {

    if (!suppressTextChanged && onTextChanged != nullptr) {
        onTextChanged();
    }
}

int FilePage::indexOf(const FileLine* line) const {

    for (int i = 0; i < (int) fileLines.size(); ++i) {
        if (fileLines[i].get() == line) {
            return i;
        }
    }

    return -1;
}

void FilePage::focusRelative(const FileLine* line, int offset) {

    const int index = indexOf(line);

    if (index < 0) {
        return;
    }

    focusLine(index + offset);
}

void FilePage::mergeWithPreviousLine(const FileLine* line) {

    if (indexOf(line) <= 0 || line == nullptr) {
        return;
    }

    juce::MessageManager::callAsync([page = juce::Component::SafePointer<FilePage>(this), line] {
        if (page != nullptr) {
            page->performMerge(line);
        }
    });
}

void FilePage::performMerge(const FileLine* line) {

    const int index = indexOf(line);

    if (index <= 0) {
        return;
    }

    LineEditor& previousEditor = *fileLines[index - 1]->lineEditor;

    const juce::String previousText = previousEditor.getLineText();
    const int          joinPosition = previousText.length();

    previousEditor.setText(previousText + fileLines[index]->getText());

    fileLines.erase(fileLines.begin() + index);

    refreshLines();

    setSize(getWidth(), preferredHeightForWidth(getWidth()));
    resized();

    focusLine(index - 1);

    fileLines[index - 1]->lineEditor->setCaretPosition(joinPosition);

    notifyTextChanged();
}

void FilePage::insertLineAfter(const FileLine* line) {

    const int index = indexOf(line);

    if (index < 0) {
        return;
    }

    LineEditor& editor = *fileLines[index]->lineEditor;

    const juce::String text  = editor.getLineText();
    const int          caret = juce::jlimit(0, text.length(), editor.getCaretPosition());

    auto newLine = std::make_unique<FileLine>(context);
    newLine->lineEditor->setText(text.substring(caret));

    editor.setText(text.substring(0, caret));

    fileLines.insert(fileLines.begin() + index + 1, std::move(newLine));

    refreshLines();

    setSize(getWidth(), preferredHeightForWidth(getWidth()));
    resized();

    focusLine(index + 1);

    fileLines[index + 1]->lineEditor->setCaretPosition(0);

    notifyTextChanged();
}

void FilePage::focusLine(int index) {

    if (index < 0 || index >= (int) fileLines.size()) {
        return;
    }

    FileLine& fileLine = *fileLines[index];

    if (auto* viewport = findParentComponentOfClass<juce::Viewport>()) {

        const juce::Rectangle<int> lineBounds = fileLine.getBounds();
        const juce::Rectangle<int> visibleArea = viewport->getViewArea();

        if (lineBounds.getY() < visibleArea.getY()) {
            viewport->setViewPosition(visibleArea.getX(), lineBounds.getY());
        }
        else if (lineBounds.getBottom() > visibleArea.getBottom()) {
            viewport->setViewPosition(visibleArea.getX(), lineBounds.getBottom() - visibleArea.getHeight());
        }
    }

    fileLine.lineEditor->beginEditing(false);
}

void FilePage::createNewFile() {

    std::vector<std::unique_ptr<FileLine>> newFileLines;
    newFileLines.reserve(initialLineCount);

    for (int i = 0; i < initialLineCount; ++i) {
        newFileLines.push_back(std::make_unique<FileLine>(context));
    }

    setFile(std::move(newFileLines));
}
