//
// Created by Eli Baumgardner on 4/12/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <limits>
#include "ValueFormat.h"
#include "../../Util/ApplicationContext.h"


class ValueEditor : public juce::Component,
                    public juce::SettableTooltipClient,
                    public juce::TextEditor::Listener,
                    public juce::Value::Listener{
public:

    explicit ValueEditor(ApplicationContext& context);
    ~ValueEditor() override;

    std::function<void()> onValueChange;

    void paint  (juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void beginEditing(bool selectAllText = true);
    void setPersistentEditor(bool shouldStayVisible);

    void bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID);

    void setFormat(std::unique_ptr<ValueFormat> newFormat);

    void enableDualValue(const juce::Identifier& secondaryPropertyID);
    void disableDualValue();
    void enableDecimalValue(double min, double max = std::numeric_limits<double>::max());
    void enableMultiplierValue(int defaultValue = 1);
    void enableTextValue();
    void setText(const juce::String& text);
    juce::String getText() const;
    void enableAutoFitText();
    void setFontHeight(float newFontHeight);
    void setPitchMode(bool shouldShowPitchNames);
    void setEditable(bool shouldBeEditable);
    void setJustification(juce::Justification newJustification);
    void setCaretColour(juce::Colour colour);
    void enableSignedValue(int min, int max);
    void disableSignedValue();
    void enablePlusRequiredValue();
    void setMinimumValue(int min);
    double clampToRange(double value) const;
    void valueChanged(juce::Value&) override;
    void commitValue();

    void acceptMultipleValues();

    std::unique_ptr<juce::TextEditor> textEditor;
    juce::Value boundValue;

protected:
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost      (juce::TextEditor& editor) override;

    juce::Font displayFont() const;

private:
    ValueBinding makeBinding() const;
    void         bindSecondaryProperties();

    juce::String getDisplayText() const;

    const ApplicationContext& applicationContext;

    std::unique_ptr<ValueFormat> format;

    juce::ValueTree  boundTree;
    juce::Identifier boundIdentifier;

    std::vector<juce::Value>      secondaryValues;
    std::vector<juce::Identifier> secondaryIdentifiers;

    juce::Justification justification { juce::Justification::centred };

    static constexpr float baseFontHeight = 9.0f;
    static constexpr float autoFitInset   = 4.0f;

    float fontHeight = baseFontHeight;

    bool isEditing        = false;
    bool persistentEditor = false;
    bool autoFitText = false;
    bool editable    = true;
};
