#include "FileLabel.h"
#include "../Theme/CustomLookAndFeel.h"

FileLabel::FileLabel(juce::UndoManager& undoManager)
    : fileText(undoManager)
{
    removeButton.icon = &CustomLookAndFeel::drawRemoveIcon;

    fileText.autoFitText = true;
    fileText.editable    = false;
    fileText.fontStyle   = Theme::FontStyle::Regular;

    fileText.setFormat(std::make_unique<TextFormat>(TextFormat::labelTextLength, TextFormat::labelCharacters));
    fileText.setInterceptsMouseClicks(false, false);

    removeButton.setTooltip("Remove Rule");

    removeButton.onClick = [this] {
        if (onRemove) {
            onRemove();
        }
    };

    addAndMakeVisible(fileText);
    addAndMakeVisible(removeButton);
}

void FileLabel::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel::get(*this).drawFileLabel(graphics, *this);
}

void FileLabel::resized()
{
    auto bounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * fileLabelInsetRatio));

    const int buttonSide = juce::roundToInt(juce::jmin(bounds.getWidth(), bounds.getHeight()) * removeButtonRatio);

    removeButton.setBounds(bounds.removeFromRight(buttonSide).withSizeKeepingCentre(buttonSide, buttonSide));

    fileText.setBounds(bounds.withSizeKeepingCentre(juce::roundToInt(bounds.getWidth() * fileTextWidthRatio),
                                                    juce::roundToInt(bounds.getHeight() * fileTextHeightRatio)));
}

void FileLabel::mouseDown(const juce::MouseEvent&)
{
    if (onMouseClicked) {
        onMouseClicked();
    }
}

void FileLabel::setGrabbed(bool shouldBeGrabbed)
{
    if (grabbed == shouldBeGrabbed) {
        return;
    }

    grabbed = shouldBeGrabbed;

    repaint();
}

void FileLabel::setSelected(bool shouldBeSelected)
{
    if (selected == shouldBeSelected) {
        return;
    }

    selected = shouldBeSelected;

    repaint();
}
