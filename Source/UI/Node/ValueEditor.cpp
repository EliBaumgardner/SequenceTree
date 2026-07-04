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
    int primaryValue = (int) boundValue.getValue();

    if (!dualNumberMode) {
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
    juce::String text = textEditor->getText();

    if (dualNumberMode) {
        commitDualValue(text);
    }
    else {
        commitSingleValue(text);
    }

    if (boundTree.isValid() && applicationContext.rtGraphBuilder != nullptr) {
        applicationContext.rtGraphBuilder->makeRTGraph(boundTree);
    }

    isEditing = false;
    textEditor->setVisible(false);
    repaint();
}

void ValueEditor::commitSingleValue(const juce::String& text)
{
    int value = text.getIntValue();
    if (value < minValue) {
        value = minValue;
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

void ValueEditor::valueChanged(juce::Value&)
{
    if (!suppressCallback) {
        repaint();
    }

    if (onValueChange)
        onValueChange();
}
