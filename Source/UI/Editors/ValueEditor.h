#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "Formats/ValueFormat.h"
#include "../../Util/ApplicationContext.h"
#include "../Theme/Theme.h"

class ValueEditor : public juce::Component,
                    public juce::SettableTooltipClient,
                    public juce::TextEditor::Listener,
                    public juce::Value::Listener
{
public:

    explicit ValueEditor(const ApplicationContext& context);
    ~ValueEditor() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    void setFormat(std::unique_ptr<ValueFormat> newFormat);
    void mouseDown(const juce::MouseEvent& event) override;
    void beginEditing(bool selectAllText = true);
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void setNumericValue(double newValue);
    void setPersistentEditor(bool shouldStayVisible);
    void bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID);
    void setFontHeight(float newFontHeight);
    void setJustification(juce::Justification newJustification);
    void commitText(const juce::String& enteredText);
    void commitValue();
    void valueChanged(juce::Value&) override;

    enum class WheelResponse { PassToParent, StepValue };
    enum class Backdrop      { None, Badge, Field, Gauge };

    static constexpr float  defaultAutoFitInsetRatio = 0.25f;
    static constexpr float  baseFontHeight           = 9.0f;
    static constexpr double wheelRangeFraction       = 1.0;

    std::function<void()> onValueChange;
    std::function<void()> onEditFinished;

    std::unique_ptr<juce::TextEditor> textEditor;
    std::unique_ptr<ValueFormat>      format;

    juce::Value boundValue;

    juce::Justification justification { juce::Justification::centred };

    float fontHeight        = baseFontHeight;
    float autoFitInsetRatio = defaultAutoFitInsetRatio;

    WheelResponse    wheelResponse = WheelResponse::PassToParent;
    Backdrop         backdrop      = Backdrop::None;
    Theme::FontStyle fontStyle     = Theme::FontStyle::Mono;

    bool autoFitText = false;
    bool editable    = true;

protected:

    juce::Font displayFont(const juce::String& text) const;
    void textEditorReturnKeyPressed(juce::TextEditor& editor) override;
    void textEditorFocusLost(juce::TextEditor& editor) override;

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
