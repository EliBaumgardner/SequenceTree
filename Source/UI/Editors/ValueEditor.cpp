//
// Created by Eli Baumgardner on 4/12/26.
//

#include "ValueEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/RTGraphBuilder.h"
#include "../../Graph/ValueTreeIdentifiers.h"

#include <cmath>

ValueEditor::ValueEditor(ApplicationContext& context) : applicationContext(context)
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

    setFormat(std::make_unique<IntFormat>());
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
    secondaryIdentifiers = format->extraProperties();

    if (! boundTree.isValid()) {
        return;
    }

    secondaryValues.reserve(secondaryIdentifiers.size());

    for (const auto& identifier : secondaryIdentifiers) {
        secondaryValues.push_back(boundTree.getPropertyAsValue(identifier, nullptr));
    }

    for (auto& value : secondaryValues) {
        value.addListener(this);
    }
}

ValueBinding ValueEditor::makeBinding() const
{
    return { boundTree, boundValue, boundIdentifier, secondaryValues, secondaryIdentifiers };
}

void ValueEditor::paint(juce::Graphics& g)
{
    if (! isEditing && ! persistentEditor) {
        g.setFont(displayFont());
        g.setColour(juce::Colours::lightgrey.withAlpha(0.85f));

        g.drawText(getDisplayText(),
                   getLocalBounds(), justification, false);
    }
}

juce::Font ValueEditor::displayFont() const
{
    juce::Font font { juce::FontOptions(fontHeight) };

    if (! autoFitText) {
        return font;
    }

    const auto bounds     = getLocalBounds().toFloat().reduced(autoFitInset);
    const float textWidth  = font.getStringWidthFloat(getDisplayText());
    const float textHeight = font.getHeight();

    if (bounds.getWidth() <= 0.0f || bounds.getHeight() <= 0.0f || textHeight <= 0.0f) {
        return font;
    }

    const float heightRatio = bounds.getHeight() / textHeight;
    float ratio        = heightRatio;

    if (textWidth > 0.0f) {
        ratio = std::min(bounds.getWidth() / textWidth, heightRatio);
    }

    const float fittedHeight = textHeight * ratio;

    if (fittedHeight <= 0.0f || ! std::isfinite(fittedHeight)) {
        return font;
    }

    font.setHeight(fittedHeight);
    return font;
}

juce::String ValueEditor::getDisplayText() const
{
    return format->displayText(makeBinding());
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

    const juce::Font font = displayFont();

    textEditor->setFont(font);
    textEditor->applyFontToAllText(font);
    textEditor->setText(format->displayText(makeBinding()), juce::dontSendNotification);
    textEditor->setVisible(true);

    repaint();
}

void ValueEditor::beginEditing(bool selectAllText)
{
    if (! editable) {
        return;
    }

    isEditing = true;

    juce::Font font = displayFont();
    textEditor->setFont(font);
    textEditor->applyFontToAllText(font);

    textEditor->setVisible(true);
    textEditor->setText(format->editText(makeBinding()), juce::dontSendNotification);
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

    boundTree  = tree;
    boundValue.referTo(tree.getPropertyAsValue(propertyID, nullptr));
    boundIdentifier = propertyID;

    boundValue.addListener(this);

    bindSecondaryProperties();

    repaint();
}

void ValueEditor::enableDualValue(const juce::Identifier& secondaryPropertyID)
{
    setFormat(std::make_unique<DualIntFormat>(secondaryPropertyID));
}

void ValueEditor::disableDualValue()
{
    if (dynamic_cast<DualIntFormat*>(format.get()) == nullptr) {
        return;
    }

    setFormat(std::make_unique<IntFormat>());
}

void ValueEditor::disablePercentValue()
{
    if (dynamic_cast<PercentFormat*>(format.get()) == nullptr) {
        return;
    }

    setFormat(std::make_unique<IntFormat>());
}

void ValueEditor::enableAutoFitText(float inset)
{
    autoFitText  = true;
    autoFitInset = inset;
}

void ValueEditor::setFontHeight(float newFontHeight)
{
    if (juce::approximatelyEqual(fontHeight, newFontHeight)) {
        return;
    }

    fontHeight = newFontHeight;

    const juce::Font font = displayFont();

    textEditor->setFont(font);
    textEditor->applyFontToAllText(font);

    repaint();
}

void ValueEditor::setPitchMode(bool shouldShowPitchNames)
{
    const bool isPitchFormat = dynamic_cast<PitchFormat*>(format.get()) != nullptr;

    if (isPitchFormat == shouldShowPitchNames) {
        return;
    }

    if (shouldShowPitchNames) {
        setFormat(std::make_unique<PitchFormat>());
    }
    else {
        setFormat(std::make_unique<IntFormat>());
    }
}

void ValueEditor::setEditable(bool shouldBeEditable)
{
    editable = shouldBeEditable;
}

void ValueEditor::setJustification(juce::Justification newJustification)
{
    justification = newJustification;

    textEditor->setJustification(newJustification);
    repaint();
}

void ValueEditor::setCaretColour(juce::Colour colour)
{
    textEditor->setColour(juce::CaretComponent::caretColourId, colour);
}

void ValueEditor::enableDecimalValue(double min, double max)
{
    setFormat(std::make_unique<DecimalFormat>());

    format->setMinimum(min);
    format->setMaximum(max);
}

void ValueEditor::enableMultiplierValue(int defaultValue)
{
    setFormat(std::make_unique<MultiplierFormat>());

    boundValue.setValue(defaultValue);
    repaint();
}

void ValueEditor::enableTextValue()
{
    setFormat(std::make_unique<TextFormat>());
}

void ValueEditor::setText(const juce::String& text)
{
    boundValue.setValue(text);

    if (persistentEditor) {
        textEditor->setText(format->displayText(makeBinding()), juce::dontSendNotification);
    }

    repaint();
}

juce::String ValueEditor::getText() const
{
    return boundValue.getValue().toString();
}

void ValueEditor::enableSignedValue(int min, int max)
{
    setFormat(std::make_unique<IntFormat>(true));

    format->setMinimum(min);
    format->setMaximum(max);
}

void ValueEditor::disableSignedValue()
{
    auto* intFormat = dynamic_cast<IntFormat*>(format.get());

    if (intFormat == nullptr || ! intFormat->showsSign()) {
        return;
    }

    setFormat(std::make_unique<IntFormat>());
}

void ValueEditor::enablePlusRequiredValue()
{
    setFormat(std::make_unique<PlusRequiredFormat>());
}

void ValueEditor::acceptMultipleValues()
{
    setFormat(std::make_unique<IntListFormat>());
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
    format->setMinimum((double) min);
}

double ValueEditor::clampToRange(double value) const
{
    return format->clamp(value);
}

void ValueEditor::commitValue()
{
    if (! isEditing) {
        return;
    }

    format->commit(textEditor->getText(), makeBinding());

    if (boundTree.isValid() && applicationContext.rtGraphBuilder != nullptr) {
        applicationContext.rtGraphBuilder->makeRTGraph(boundTree);
    }

    isEditing = false;

    if (! persistentEditor) {
        textEditor->setVisible(false);
    }

    repaint();
}

void ValueEditor::valueChanged(juce::Value&)
{
    repaint();

    if (onValueChange) {
        onValueChange();
    }
}
