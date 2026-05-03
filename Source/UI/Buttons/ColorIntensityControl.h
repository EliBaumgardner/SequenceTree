//
// Created by Eli Baumgardner on 4/14/26.
//

#ifndef SEQUENCETREE_COLORINTENSITYCONTROL_H
#define SEQUENCETREE_COLORINTENSITYCONTROL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Theme/CustomLookAndFeel.h"
#include "../../Util/ApplicationContext.h"

class ColorIntensityControl : public juce::Component,
                              public juce::SettableTooltipClient,
                              public juce::TextEditor::Listener {

    ApplicationContext& applicationContext;

public:

    ColorIntensityControl(ApplicationContext& context) : applicationContext(context)
    {
        setLookAndFeel(applicationContext.lookAndFeel);
        setTooltip("Color Intensity");

        textEditor = std::make_unique<juce::TextEditor>();
        textEditor->addListener(this);
        textEditor->setMultiLine(false);
        textEditor->setReturnKeyStartsNewLine(false);
        textEditor->setInputRestrictions(4, "0123456789.");
        textEditor->setJustification(juce::Justification::centred);
        textEditor->setColour(juce::TextEditor::backgroundColourId,     juce::Colours::transparentBlack);
        textEditor->setColour(juce::TextEditor::outlineColourId,        juce::Colours::transparentBlack);
        textEditor->setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::transparentBlack);
        textEditor->setColour(juce::TextEditor::textColourId,           juce::Colours::lightgrey);
        textEditor->setFont(juce::Font(juce::FontOptions(9.0f)));
        textEditor->setVisible(false);
        addChildComponent(textEditor.get());
    }

    void paint(juce::Graphics& g) override
    {
        if (!isEditing)
        {
            g.setFont(juce::Font(juce::FontOptions(9.0f)));
            g.setColour(juce::Colours::lightgrey.withAlpha(0.85f));
            g.drawText(juce::String(applicationContext.lookAndFeel->getColorIntensityFactor(), 2),
                       getLocalBounds(), juce::Justification::centred, false);
        }
    }

    void resized() override
    {
        textEditor->setBounds(getLocalBounds());
    }

    void mouseDown(const juce::MouseEvent&) override
    {
        isDragging = false;
        dragStartValue = applicationContext.lookAndFeel->getColorIntensityFactor();
    }

    void mouseDrag(const juce::MouseEvent& e) override
    {
        isDragging = true;
        int yOffset = e.getOffsetFromDragStart().y;
        float delta = -yOffset * 0.005f;
        float newValue = juce::jlimit(0.0f, 1.0f, dragStartValue + delta);
        applicationContext.lookAndFeel->setColorIntensityFactor(newValue);
        if (applicationContext.canvas != nullptr)
            if (auto* topLevel = applicationContext.canvas->getTopLevelComponent())
                topLevel->repaint();
        repaint();
    }

    void mouseUp(const juce::MouseEvent&) override
    {
        if (!isDragging)
        {
            isEditing = true;
            textEditor->setVisible(true);
            textEditor->setText(juce::String(applicationContext.lookAndFeel->getColorIntensityFactor(), 2),
                                juce::dontSendNotification);
            textEditor->grabKeyboardFocus();
            textEditor->selectAll();
            repaint();
        }
    }

    void textEditorReturnKeyPressed(juce::TextEditor&) override { commitValue(); }
    void textEditorFocusLost(juce::TextEditor&) override        { commitValue(); }

private:

    void commitValue()
    {
        float value = textEditor->getText().getFloatValue();
        applicationContext.lookAndFeel->setColorIntensityFactor(value);
        if (applicationContext.canvas != nullptr)
            if (auto* topLevel = applicationContext.canvas->getTopLevelComponent())
                topLevel->repaint();

        isEditing = false;
        textEditor->setVisible(false);
        repaint();
    }

    std::unique_ptr<juce::TextEditor> textEditor;
    bool isEditing = false;
    bool isDragging = false;
    float dragStartValue = 1.0f;
};

#endif //SEQUENCETREE_COLORINTENSITYCONTROL_H
