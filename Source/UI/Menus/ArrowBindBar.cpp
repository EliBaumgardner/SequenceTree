//
// Created by Eli Baumgardner on 7/27/26.
//

#include "ArrowBindBar.h"
#include "../Theme/CustomLookAndFeel.h"

ArrowBindBar::ArrowBindBar(ApplicationContext& context)
    : Bar(context, { Orientation::horizontal, Background::litFromBottom })
{
    configureRow(pitchRow,    "bind to pitch");
    configureRow(durationRow, "bind to duration");
}

std::unique_ptr<IconButton> ArrowBindBar::createToggle(const juce::String& text)
{
    auto toggle = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawTextButton(g, bounds, state);
        }, applicationContext.lookAndFeel);

    toggle->setText(text);
    toggle->onClick = [button = toggle.get()]() { button->toggleSelected(); };

    return toggle;
}

void ArrowBindBar::configureRow(BindRow& row, const juce::String& labelText)
{
    row.xToggle = createToggle("X");
    row.yToggle = createToggle("Y");

    row.editor = std::make_unique<ValueEditor>(applicationContext);

    row.label.setText(labelText, juce::dontSendNotification);
    row.label.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    row.label.setFont(juce::Font(juce::FontOptions(Theme::labelFontHeight)));
    row.label.setJustificationType(juce::Justification::centredLeft);

    addAndMakeVisible(row.label);
    addAndMakeVisible(*row.editor);
    addAndMakeVisible(*row.xToggle);
    addAndMakeVisible(*row.yToggle);
}

void ArrowBindBar::layOutRow(BindRow& row, juce::Rectangle<int> bounds)
{
    row.yToggle->setBounds(bounds.removeFromRight(toggleWidth));
    bounds.removeFromRight(itemGap);

    row.xToggle->setBounds(bounds.removeFromRight(toggleWidth));
    bounds.removeFromRight(itemGap);

    row.editor->setBounds(bounds.removeFromRight(editorWidth));
    bounds.removeFromRight(itemGap);

    row.label.setBounds(bounds);
}

void ArrowBindBar::resized()
{
    auto bounds = getContentBounds();

    layOutRow(pitchRow,    bounds.removeFromTop(rowHeight));
    layOutRow(durationRow, bounds.removeFromTop(rowHeight));
}
