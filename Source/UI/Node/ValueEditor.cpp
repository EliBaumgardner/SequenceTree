//
// Created by Eli Baumgardner on 4/12/26.
//

#include "ValueEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Graph/ValueTreeIdentifiers.h"

ValueEditor::ValueEditor(ApplicationContext& context) : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    textEditor = std::make_unique<juce::TextEditor>();

    textEditor->addListener(this);

    textEditor->setMultiLine(false);
    textEditor->setReturnKeyStartsNewLine(false);

    textEditor->setInputRestrictions(4, "0123456789");

    textEditor->setJustification(juce::Justification::centred);

    textEditor->setColour(juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::textColourId,           juce::Colours::lightgrey);

    textEditor->setFont(juce::Font(juce::FontOptions(9.0f)));
    textEditor->setVisible(false);

    addChildComponent(textEditor.get());

    boundValue.addListener(this);
}

void ValueEditor::paint(juce::Graphics& g)
{
    if (!isEditing) {
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(juce::Colours::lightgrey.withAlpha(0.85f));

        g.drawText(getDisplayText(),
                   getLocalBounds(), juce::Justification::centred, false);
    }
}

juce::String ValueEditor::getDisplayText() const
{
    if (decimalMode) {
        double value = (double) boundValue.getValue();
        juce::String text(value, 3);
        text = text.trimCharactersAtEnd("0").trimCharactersAtEnd(".");
        return text.isEmpty() ? "0" : text;
    }

    int primaryValue = (int) boundValue.getValue();

    if (requirePlusMode) {
        if (primaryValue > 0) {
            return "+" + juce::String(primaryValue);
        }
        if (primaryValue < 0) {
            return juce::String(primaryValue);
        }
        return juce::String();
    }

    if (!dualNumberMode) {
        if (acceptMultiple) {
            return juce::String(editorText);
        }
        if (signedMode && primaryValue > 0) {
            return "+" + juce::String(primaryValue);
        }
        return juce::String(primaryValue);
    }

    int secondaryValue = (int) boundSecondaryValue.getValue();

    if (secondaryValue <= 0) {
        return juce::String(primaryValue);
    }

    return juce::String(primaryValue) + ":" + juce::String(secondaryValue);
}

void ValueEditor::resized()
{
    textEditor->setBounds(getLocalBounds());
}

void ValueEditor::mouseDown(const juce::MouseEvent&)
{
    isEditing = true;
    textEditor->setVisible(true);
    textEditor->setText(getDisplayText(), juce::dontSendNotification);
    textEditor->grabKeyboardFocus();
    textEditor->selectAll();
    repaint();
}

void ValueEditor::bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID)
{
    boundValue.removeListener(this);
    boundSecondaryValue.removeListener(this);

    boundTree  = tree;
    boundValue.referTo(tree.getPropertyAsValue(propertyID, nullptr));
    boundIdentifier = propertyID;

    boundValue.addListener(this);

    if (dualNumberMode && secondaryIdentifier.isValid()) {
        boundSecondaryValue.referTo(tree.getPropertyAsValue(secondaryIdentifier, nullptr));
        boundSecondaryValue.addListener(this);
    }

    repaint();
}

void ValueEditor::enableDualValue(const juce::Identifier& secondaryPropertyID)
{
    dualNumberMode      = true;
    secondaryIdentifier = secondaryPropertyID;

    textEditor->setInputRestrictions(9, "0123456789:");
}

void ValueEditor::enableDecimalValue(double min, double max)
{
    decimalMode     = true;
    minDecimalValue = min;
    maxDecimalValue = max;

    textEditor->setInputRestrictions(6, "0123456789.");
}

void ValueEditor::enableSignedValue(int min, int max)
{
    signedMode = true;
    minValue   = min;
    maxValue   = max;

    textEditor->setInputRestrictions(4, "-0123456789");
}

void ValueEditor::enablePlusRequiredValue()
{
    requirePlusMode = true;

    textEditor->setInputRestrictions(5, "+-0123456789");
}

void ValueEditor::textEditorReturnKeyPressed(juce::TextEditor&)
{
    commitValue();
}

void ValueEditor::textEditorFocusLost(juce::TextEditor&)
{
    commitValue();
}

void ValueEditor::setMinimumValue(int min)
{
    minValue = min;
}

void ValueEditor::commitValue()
{
    editorText = textEditor->getText();

    if (dualNumberMode) {
        commitDualValue(editorText);
    }
    else if (acceptMultiple){
        DBG("acceptMultiple");
        commitMultipleValues(editorText);
    }
    else {
        commitSingleValue(editorText);
    }

    if (boundTree.isValid() && applicationContext.rtGraphBuilder != nullptr) {
        applicationContext.rtGraphBuilder->makeRTGraph(boundTree);
    }

    isEditing = false;
    textEditor->setVisible(false);
    repaint();
}

void ValueEditor::acceptMultipleValues() {

    acceptMultiple = true;
    textEditor->setInputRestrictions(8, "0123456789' ',");
}

void ValueEditor::commitSingleValue(const juce::String& text)
{
    if (decimalMode) {
        double value = text.getDoubleValue();
        if (value < minDecimalValue) {
            value = minDecimalValue;
        }
        if (value > maxDecimalValue) {
            value = maxDecimalValue;
        }

        boundValue.setValue(value);
        return;
    }

    if (requirePlusMode) {
        juce::String trimmed = text.trim();

        bool spawns  = trimmed.startsWithChar('+');
        bool removes = trimmed.startsWithChar('-');

        if (!spawns && !removes) {
            boundValue.setValue(0);
            return;
        }

        int magnitude = trimmed.substring(1).getIntValue();
        if (magnitude < minValue) {
            magnitude = minValue;
        }
        if (magnitude > maxValue) {
            magnitude = maxValue;
        }

        boundValue.setValue(removes ? -magnitude : magnitude);
        return;
    }

    int value = text.getIntValue();
    if (value < minValue) {
        value = minValue;
    }
    if (value > maxValue) {
        value = maxValue;
    }

    boundValue.setValue(value);
}

void ValueEditor::commitDualValue(const juce::String& text)
{
    int separatorIndex = text.indexOfChar(':');

    juce::String primaryText   = text;
    juce::String secondaryText;

    if (separatorIndex >= 0) {
        primaryText   = text.substring(0, separatorIndex);
        secondaryText = text.substring(separatorIndex + 1);
    }

    int primaryValue = primaryText.getIntValue();
    if (primaryValue < minValue) {
        primaryValue = minValue;
    }

    int secondaryValue = 0;
    if (secondaryText.isNotEmpty()) {
        secondaryValue = secondaryText.getIntValue();
        if (secondaryValue < 0) {
            secondaryValue = 0;
        }
    }

    boundValue.setValue(primaryValue);
    boundSecondaryValue.setValue(secondaryValue);
}

void ValueEditor::commitMultipleValues(const juce::String &text) {
    DBG(text);

    for (int i = 0; i < text.length(); i++) {
        char c = text[i];
        if (c != ' ' && c != ',') {
            DBG(c);
            juce::Value value;
            value.setValue(c);
            values.push_back(value);
        }
    }

    onValueChange();
}

void ValueEditor::valueChanged(juce::Value&)
{
    if (!suppressCallback) {
        repaint();
    }

    if (onValueChange)
        onValueChange();
}
