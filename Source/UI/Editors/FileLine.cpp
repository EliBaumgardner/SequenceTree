//
// Created by Eli Baumgardner on 8/19/26.
//

#include "FileLine.h"
#include "LineEditor.h"
#include "../Theme/CustomLookAndFeel.h"


FileLine::FileLine(ApplicationContext& context) : context(context) {

    lineEditor = std::make_unique<LineEditor>(context);
    addAndMakeVisible(lineEditor.get());
}

FileLine::~FileLine() = default;

void FileLine::setLineNumber(int newLineNumber) {

    if (lineNumber == newLineNumber) {
        return;
    }

    lineNumber = newLineNumber;
    repaint();
}

void FileLine::setGutterWidth(int newGutterWidth) {

    if (gutterWidth == newGutterWidth) {
        return;
    }

    gutterWidth = newGutterWidth;

    resized();
    repaint();
}

void FileLine::setFontHeight(float newFontHeight) {

    if (juce::approximatelyEqual(fontHeight, newFontHeight)) {
        return;
    }

    fontHeight = newFontHeight;
    lineEditor->setFontHeight(newFontHeight);

    repaint();
}

void FileLine::setRowHeight(int newRowHeight) {

    if (rowHeight == newRowHeight) {
        return;
    }

    rowHeight = newRowHeight;
    repaint();
}

juce::String FileLine::getText() const {

    return lineEditor->getLineText();
}

int FileLine::editorWidthFor(int lineWidth) const {

    return lineWidth - gutterWidth - contentInset * 2;
}

void FileLine::paint(juce::Graphics &g) {

    const Theme& theme = CustomLookAndFeel::get(*this);

    g.setColour(juce::Colours::black);
    g.drawRect(getLocalBounds(), 1.0f);

    if (gutterWidth <= 0) {
        return;
    }

    g.setColour(theme.getTextColour().withAlpha(0.12f));
    g.drawVerticalLine(gutterWidth, 0.0f, (float) getHeight());

    if (lineNumber <= 0) {
        return;
    }

    g.setColour(theme.lineNumberColour);
    g.setFont(juce::Font(juce::FontOptions(fontHeight)));

    juce::Rectangle<int> numberBounds = getLocalBounds().withWidth(gutterWidth).withTrimmedLeft(gutterTextInset);

    if (rowHeight > 0) {
        numberBounds = numberBounds.withHeight(rowHeight);
    }

    g.drawText(juce::String(lineNumber), numberBounds, juce::Justification::centredLeft, false);
}

void FileLine::resized() {

    auto bounds = getLocalBounds();
    bounds.removeFromLeft(gutterWidth);

    lineEditor->setBounds(bounds.reduced(contentInset, 0));
}
