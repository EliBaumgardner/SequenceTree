//
// Created by Eli Baumgardner on 4/12/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../../Util/ApplicationContext.h"


class ValueEditor : public juce::Component,
                    public juce::SettableTooltipClient,
                    public juce::TextEditor::Listener,
                    public juce::Value::Listener{
public:

    std::function<void()> onValueChanged;

    ValueEditor(ApplicationContext& context);

    std::function<void()> onValueChange;

    void paint  (juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID);

    void enableDualValue(const juce::Identifier& secondaryPropertyID);
    void setMinimumValue(int min);
    void valueChanged(juce::Value&) override;
    void commitValue();

    std::unique_ptr<juce::TextEditor> textEditor;
    juce::Value boundValue;

private:
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost      (juce::TextEditor& editor) override;

    juce::String getDisplayText() const;
    void commitSingleValue(const juce::String& text);
    void commitDualValue  (const juce::String& text);

    ApplicationContext& applicationContext;

    juce::Value boundSecondaryValue;

    juce::Identifier boundIdentifier;
    juce::Identifier secondaryIdentifier;

    juce::ValueTree boundTree;


    bool isEditing        = false;
    bool suppressCallback = false;
    bool dualNumberMode   = false;
    int  minValue         = 1;
};
