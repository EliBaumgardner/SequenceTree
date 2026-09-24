//
// Created by Eli Baumgardner on 4/12/26.
//

#include "ValueEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../../Util/NodeInfo.h"

#include <cmath>

ValueEditor::ValueEditor(const ApplicationContext& context) : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    textEditor = std::make_unique<juce::TextEditor>();

    textEditor->addListener(this);

    textEditor->setMultiLine(false);
    textEditor->setReturnKeyStartsNewLine(false);

    textEditor->setJustification(juce::Justification::centred);

    textEditor->setBorder(juce::BorderSize<int>(0));
    textEditor->setIndents(0, 0);

    textEditor->setColour(juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
    textEditor->setColour(juce::TextEditor::textColourId,           juce::Colours::lightgrey);

    textEditor->setFont(juce::Font(juce::FontOptions(baseFontHeight)));
    textEditor->setVisible(false);

    addChildComponent(textEditor.get());

    boundValue.addListener(this);

    setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));
}

ValueEditor::~ValueEditor()
{
    boundValue.removeListener(this);

    for (auto& value : secondaryValues) {
        value.removeListener(this);
    }

    setLookAndFeel(nullptr);
}

void ValueEditor::setFormat(std::unique_ptr<ValueFormat> newFormat)
{
    format = std::move(newFormat);

    const InputRestrictions restrictions = format->restrictions();
    textEditor->setInputRestrictions(restrictions.maxLength, restrictions.allowedCharacters);

    bindSecondaryProperties();
    repaint();
}

void ValueEditor::bindSecondaryProperties()
{
    for (auto& value : secondaryValues) {
        value.removeListener(this);
    }

    secondaryValues.clear();

    if (! boundTree.isValid()) {
        return;
    }

    secondaryValues.reserve(format->extraProperties.size());

    for (const auto& identifier : format->extraProperties) {
        secondaryValues.push_back(boundTree.getPropertyAsValue(identifier, applicationContext.undoManager));
    }

    for (auto& value : secondaryValues) {
        value.addListener(this);
    }
}

void ValueEditor::paint(juce::Graphics& g)
{
    if (! isEditing && ! persistentEditor) {
        const juce::String displayed = format->text(binding, TextPurpose::Display);

        g.setFont(displayFont(displayed));
        g.setColour(juce::Colours::lightgrey.withAlpha(0.85f));

        g.drawText(displayed, getLocalBounds(), justification, false);
    }
}

juce::Font ValueEditor::displayFont(const juce::String& text) const
{
    juce::Font font { juce::FontOptions(fontHeight) };

    if (! autoFitText) {
        return font;
    }

    const float inset      = (float) getHeight() * autoFitInsetRatio;
    const auto  bounds     = getLocalBounds().toFloat().reduced(inset);
    const float textWidth  = font.getStringWidthFloat(text);
    const float textHeight = font.getHeight();

    if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f || textHeight <= 0.0f) {
        return font;
    }

    float fittedHeight = bounds.getHeight();

    if (textWidth > 0.0f) {
        fittedHeight = std::min(textHeight * (bounds.getWidth() / textWidth), fittedHeight);
    }

    if (fittedHeight <= 0.0f || ! std::isfinite(fittedHeight)) {
        return font;
    }

    font.setHeight(fittedHeight);
    return font;
}

void ValueEditor::resized()
{
    textEditor->setBounds(getLocalBounds());
}

void ValueEditor::mouseDown(const juce::MouseEvent&)
{
    beginEditing();
}

void ValueEditor::setPersistentEditor(bool shouldStayVisible)
{
    persistentEditor = shouldStayVisible;

    if (! persistentEditor) {
        return;
    }

    const juce::String displayed = format->text(binding, TextPurpose::Display);
    const juce::Font   font      = displayFont(displayed);

    textEditor->setFont(font);
    textEditor->applyFontToAllText(font);
    textEditor->setText(displayed, juce::dontSendNotification);
    textEditor->setVisible(true);

    repaint();
}

void ValueEditor::beginEditing(bool selectAllText)
{
    if (! editable) {
        return;
    }

    isEditing = true;

    const juce::Font font = displayFont(format->text(binding, TextPurpose::Display));

    textEditor->setFont(font);
    textEditor->applyFontToAllText(font);

    textEditor->setVisible(true);
    textEditor->setText(format->text(binding, TextPurpose::Editing), juce::dontSendNotification);
    textEditor->grabKeyboardFocus();

    if (selectAllText) {
        textEditor->selectAll();
    }
    else {
        textEditor->setCaretPosition(textEditor->getTotalNumChars());
    }

    repaint();
}

void ValueEditor::bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID)
{
    boundValue.removeListener(this);

    boundTree       = tree;
    boundIdentifier = propertyID;
    boundValue.referTo(tree.getPropertyAsValue(propertyID, applicationContext.undoManager));

    boundValue.addListener(this);

    bindSecondaryProperties();

    repaint();
}

void ValueEditor::setFontHeight(float newFontHeight)
{
    if (juce::approximatelyEqual(fontHeight, newFontHeight)) {
        return;
    }

    fontHeight = newFontHeight;

    const juce::Font font = displayFont(format->text(binding, TextPurpose::Display));

    textEditor->setFont(font);
    textEditor->applyFontToAllText(font);

    repaint();
}

void ValueEditor::setJustification(juce::Justification newJustification)
{
    justification = newJustification;

    textEditor->setJustification(newJustification);
    repaint();
}

void ValueEditor::commitText(const juce::String& enteredText)
{
    const ParsedValue parsed = format->parse(enteredText);

    if (boundTree.isValid() && applicationContext.undoManager != nullptr) {
        applicationContext.undoManager->beginNewTransaction();
    }

    boundValue.setValue(parsed.primary);

    for (size_t i = 0; i < secondaryValues.size() && i < parsed.secondaries.size(); i++) {
        secondaryValues[i].setValue(parsed.secondaries[i]);
    }

    if (persistentEditor) {
        textEditor->setText(format->text(binding, TextPurpose::Display), juce::dontSendNotification);
    }

    repaint();
}

void ValueEditor::textEditorReturnKeyPressed(juce::TextEditor&)
{
    commitValue();
}

void ValueEditor::textEditorFocusLost(juce::TextEditor&)
{
    commitValue();
}

void ValueEditor::commitValue()
{
    if (! isEditing) {
        return;
    }

    const juce::String entered = textEditor->getText();

    isEditing = false;

    if (! persistentEditor) {
        textEditor->setVisible(false);
    }

    if (entered != format->text(binding, TextPurpose::Editing)) {
        const ParsedValue parsed = format->parse(entered);

        if (boundTree.isValid() && applicationContext.undoManager != nullptr) {
            applicationContext.undoManager->beginNewTransaction();
        }

        boundValue.setValue(parsed.primary);

        for (size_t i = 0; i < secondaryValues.size() && i < parsed.secondaries.size(); i++) {
            secondaryValues[i].setValue(parsed.secondaries[i]);
        }
    }

    repaint();

    if (onEditFinished) {
        onEditFinished();
    }
}

void ValueEditor::setNumericValue(double newValue)
{
    const double clamped = juce::jlimit(format->minimum, format->maximum, newValue);

    if (format->decimalPlaces > 0) {
        boundValue.setValue(clamped);
        return;
    }

    boundValue.setValue(juce::roundToInt(clamped));
}

void ValueEditor::valueChanged(juce::Value&)
{
    repaint();

    if (onValueChange) {
        onValueChange();
    }
}
