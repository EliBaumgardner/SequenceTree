//
// Created by Eli Baumgardner on 4/12/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class ValueEditor : public juce::Component,
                    public juce::SettableTooltipClient,
                    public juce::TextEditor::Listener {
public:
    ValueEditor();

    void paint  (juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID);
    void setMinimumValue(int min);

private:
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost      (juce::TextEditor& editor) override;
    void commitValue();

    juce::Value boundValue;
    juce::ValueTree boundTree;
    std::unique_ptr<juce::TextEditor> textEditor;
    bool isEditing  = false;
    int  minValue   = 1;
};
