//
// Created by Eli Baumgardner on 8/23/26.
//

#ifndef SEQUENCETREE_LINEEDITOR_H
#define SEQUENCETREE_LINEEDITOR_H

#include "ValueEditor.h"

class LineEditor : public ValueEditor,
                   public juce::KeyListener {
public:

    explicit LineEditor(ApplicationContext& context);
    ~LineEditor() override;

    juce::String getLineText() const;

    int  getCaretPosition() const;
    void setCaretPosition(int position);
    void indent(int indentSize);

    std::function<void()> onInsertLine;
    std::function<void()> onMergeWithPreviousLine;
    std::function<void()> onMoveToNextLine;
    std::function<void()> onMoveToPreviousLine;
    std::function<void()> onWrapChanged;
    std::function<void()> onIndent;

private:

    bool keyPressed(const juce::KeyPress& key, juce::Component* originatingComponent) override;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorTextChanged(juce::TextEditor& editor) override;

    void moveToNextLine();
    void moveToPreviousLine();

     const int indentSize = 10;
};


#endif //SEQUENCETREE_LINEEDITOR_H
