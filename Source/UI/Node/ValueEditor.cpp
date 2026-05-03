//
// Created by Eli Baumgardner on 4/12/26.
//

#include "ValueEditor.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Graph/RTGraphBuilder.h"

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
}

void ValueEditor::paint(juce::Graphics& g)
{
    if (!isEditing)
    {
        g.setFont(juce::Font(juce::FontOptions(9.0f)));
        g.setColour(juce::Colours::lightgrey.withAlpha(0.85f));
        // Cast to int so it never shows as "2.0"
        g.drawText(juce::String((int) boundValue.getValue()),
                   getLocalBounds(), juce::Justification::centred, false);
    }
}

void ValueEditor::resized()
{
    textEditor->setBounds(getLocalBounds());
}

void ValueEditor::mouseDown(const juce::MouseEvent&)
{
    isEditing = true;
    textEditor->setVisible(true);
    textEditor->setText(juce::String((int) boundValue.getValue()), juce::dontSendNotification);
    textEditor->grabKeyboardFocus();
    textEditor->selectAll();
    repaint();
}

void ValueEditor::bindEditor(juce::ValueTree tree, const juce::Identifier& propertyID)
{
    boundTree  = tree;
    boundValue.referTo(tree.getPropertyAsValue(propertyID, nullptr));
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

void ValueEditor::setMinimumValue(int min)
{
    minValue = min;
}

void ValueEditor::commitValue()
{
    int val = textEditor->getText().getIntValue();
    if (val < minValue) val = minValue;
    boundValue.setValue(val);

    if (boundTree.isValid() && applicationContext.rtGraphBuilder != nullptr)
        applicationContext.rtGraphBuilder->makeRTGraph(boundTree);

    isEditing = false;
    textEditor->setVisible(false);
    repaint();
}
