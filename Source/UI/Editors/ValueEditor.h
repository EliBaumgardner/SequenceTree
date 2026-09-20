//
// Created by Eli Baumgardner on 4/12/26.
//

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ValueFormat.h"
#include "../../Util/ApplicationContext.h"


class ValueEditor : public juce::Component,
                    public juce::SettableTooltipClient,
                    public juce::TextEditor::Listener,
                    public juce::Value::Listener{
public:

    explicit ValueEditor(ApplicationContext& context);
    ~ValueEditor() override;

    void setFormat(std::unique_ptr<ValueFormat> newFormat);

    void paint  (juce::Graphics& g) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& e) override;

    void setPersistentEditor(bool shouldStayVisible);
    void beginEditing(bool selectAllText = true);

    void bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID);

    void setFontHeight(float newFontHeight);
    void setJustification(juce::Justification newJustification);

    void commitText(const juce::String& enteredText);
    void commitValue();
    void setNumericValue(double newValue);

    void valueChanged(juce::Value&) override;

    static constexpr float defaultAutoFitInsetRatio = 0.25f;
    static constexpr float baseFontHeight           = 9.0f;

    std::function<void()> onValueChange;
    std::function<void()> onEditFinished;

    std::unique_ptr<juce::TextEditor> textEditor;
    std::unique_ptr<ValueFormat>      format;

    juce::Value boundValue;

    juce::Justification justification { juce::Justification::centred };

    float fontHeight        = baseFontHeight;
    float autoFitInsetRatio = defaultAutoFitInsetRatio;

    bool autoFitText = false;
    bool editable    = true;

protected:
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost      (juce::TextEditor& editor) override;

    juce::Font displayFont(const juce::String& text) const;

private:
    void bindSecondaryProperties();

    const ApplicationContext& applicationContext;

    juce::ValueTree  boundTree;
    juce::Identifier boundIdentifier;

    std::vector<juce::Value> secondaryValues;

    ValueBinding binding { boundValue, secondaryValues };

    bool isEditing        = false;
    bool persistentEditor = false;
};
