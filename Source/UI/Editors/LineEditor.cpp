//
// Created by Eli Baumgardner on 8/23/26.
//

#include "LineEditor.h"

LineEditor::LineEditor(ApplicationContext& context) : ValueEditor(context)
{
    setFormat(std::make_unique<FreeTextFormat>());
    setJustification(juce::Justification::topLeft);
    setCaretColour(juce::Colours::white);

    textEditor->setMultiLine(true, true);
    textEditor->setReturnKeyStartsNewLine(false);
    textEditor->setScrollbarsShown(false);

    setPersistentEditor(true);

    textEditor->addKeyListener(this);
}

LineEditor::~LineEditor()
{
    textEditor->removeKeyListener(this);
}

juce::String LineEditor::getLineText() const
{
    return textEditor->getText();
}

int LineEditor::getCaretPosition() const
{
    return textEditor->getCaretPosition();
}

void LineEditor::setCaretPosition(int position)
{
    textEditor->setCaretPosition(position);
}

bool LineEditor::keyPressed(const juce::KeyPress& key, juce::Component*)
{
    if (key == juce::KeyPress::upKey) {
        moveToPreviousLine();
        return true;
    }

    if (key == juce::KeyPress::downKey) {
        moveToNextLine();
        return true;
    }

    if (key == juce::KeyPress::tabKey) {
        indent(indentSize);
        return true;
    }

    if (key == juce::KeyPress::backspaceKey
        && textEditor->getCaretPosition() == 0
        && textEditor->getHighlightedRegion().isEmpty()
        && onMergeWithPreviousLine) {

        onMergeWithPreviousLine();
        return true;
    }

    return false;
}

void LineEditor::indent(int indentSize) {

    textEditor->insertTextAtCaret(juce::String::repeatedString(" ", indentSize));
}

void LineEditor::textEditorReturnKeyPressed(juce::TextEditor&)
{
    if (onInsertLine) {
        onInsertLine();
    }
}

void LineEditor::textEditorTextChanged(juce::TextEditor&)
{
    boundValue.setValue(textEditor->getText());

    if (onWrapChanged) {
        onWrapChanged();
    }
}

void LineEditor::moveToNextLine()
{
    if (onMoveToNextLine) {
        onMoveToNextLine();
    }
}

void LineEditor::moveToPreviousLine()
{
    if (onMoveToPreviousLine) {
        onMoveToPreviousLine();
    }
}
