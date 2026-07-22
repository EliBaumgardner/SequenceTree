//
// Created by Eli Baumgardner on 4/12/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
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
    void disableDualValue();
    void enableDecimalValue(double min, double max = std::numeric_limits<double>::max());
    void enableAutoFitText();
    void setPitchMode(bool shouldShowPitchNames);
    void setEditable(bool shouldBeEditable);
    void enableSignedValue(int min, int max);
    void enablePlusRequiredValue();
    void setMinimumValue(int min);
    void valueChanged(juce::Value&) override;
    void commitValue();

    void acceptMultipleValues();

    std::unique_ptr<juce::TextEditor> textEditor;
    juce::Value boundValue;

    std::vector<juce::Value> values;

    juce::String editorText;

private:
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost      (juce::TextEditor& editor) override;

    juce::String getDisplayText() const;
    juce::Font   displayFont() const;
    void commitSingleValue(const juce::String& text);
    void commitDualValue  (const juce::String& text);
    void commitMultipleValues(const juce::String& text);

    ApplicationContext& applicationContext;

    juce::Value boundSecondaryValue;

    juce::Identifier boundIdentifier;
    juce::Identifier secondaryIdentifier;

    juce::ValueTree boundTree;


    bool isEditing        = false;
    bool suppressCallback = false;
    bool dualNumberMode   = false;
    bool decimalMode      = false;
    bool acceptMultiple   = false;
    bool pitchMode        = false;
    bool autoFitText      = false;
    bool editable         = true;

    static constexpr float baseFontHeight = 9.0f;
    static constexpr float autoFitInset   = 4.0f;

    int  minValue         = 1;
    int  maxValue         = std::numeric_limits<int>::max();
    bool signedMode       = false;
    bool requirePlusMode  = false;
    double minDecimalValue = 0.1;
    double maxDecimalValue = std::numeric_limits<double>::max();
};
