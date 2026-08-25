//
// Created by Eli Baumgardner on 8/16/26.
//

#include "FileLabel.h"
#include "../Theme/CustomLookAndFeel.h"

FileLabel::FileLabel(ApplicationContext& context) : context(context) {

    setLookAndFeel(context.lookAndFeel);
    fileText = std::make_unique<ValueEditor>(context);
    fileText->enableTextValue();
    fileText->enableAutoFitText();
    fileText->setCaretColour(juce::Colours::lightgrey);

    removeButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawRemoveIcon(g, bounds, state);
        }, context.lookAndFeel);

    removeButton->setTooltip("Remove Rule");

    removeButton->onClick = [this] {
        if (onRemove) {
            onRemove();
        }
    };

    addAndMakeVisible(fileText.get());
    addAndMakeVisible(removeButton.get());
}

void FileLabel::paint(juce::Graphics &g) {

    CustomLookAndFeel::get(*this).drawFileLabel(g,*this);
}

void FileLabel::resized()
{
    auto bounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * contentInsetRatio));

    const int buttonSide = juce::roundToInt(juce::jmin(bounds.getWidth(), bounds.getHeight()) * removeButtonRatio);

    removeButton->setBounds(bounds.removeFromRight(buttonSide).withSizeKeepingCentre(buttonSide, buttonSide));

    fileText->setBounds(bounds.withSizeKeepingCentre(
        juce::roundToInt(bounds.getWidth()  * fileTextWidthRatio),
        juce::roundToInt(bounds.getHeight() * fileTextHeightRatio)));
}

void FileLabel::mouseDown(const juce::MouseEvent &event)
{
    DBG("file label clicked");

    if (onMouseClicked) {
        onMouseClicked();
    }
}

void FileLabel::setFileName(const juce::String fileName)
{
    fileText->setText(fileName);
}

juce::String FileLabel::getFileName() const
{
    return fileText->getText();
}

void FileLabel::setSelected(bool shouldBeSelected)
{
    if (selected == shouldBeSelected) {
        return;
    }

    selected = shouldBeSelected;
    repaint();
}

void FileLabel::setGrabbed(bool shouldBeGrabbed)
{
    if (grabbed == shouldBeGrabbed) {
        return;
    }

    grabbed = shouldBeGrabbed;
    repaint();
}
