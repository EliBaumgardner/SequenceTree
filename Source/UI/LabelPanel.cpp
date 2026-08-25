//
// Created by Eli Baumgardner on 8/16/26.
//

#include "LabelPanel.h"
#include "../UI/Theme/CustomLookAndFeel.h"

LabelPanel::LabelPanel(ApplicationContext &context) : context(context) {

    setLookAndFeel(context.lookAndFeel);
}

void LabelPanel::paint(juce::Graphics &g) {
    auto bounds = getLocalBounds();
    g.setColour(CustomLookAndFeel::get(*this).baseDarkColour2);
    g.fillRect(bounds);
}

void LabelPanel::resized() {
    labelHeight = juce::roundToInt(getWidth() * labelAspectRatio);
    labelGap    = juce::roundToInt(labelHeight * labelGapRatio);

    auto bounds = getLocalBounds();

    for (auto& label : labels) {
        label->setBounds(bounds.removeFromTop(labelHeight));
        bounds.removeFromTop(labelGap);
    }
}

int LabelPanel::labelIndexAt(int y) const {
    if (labels.empty() || labelHeight <= 0)
        return -1;

    return juce::jlimit(0, (int) labels.size() - 1, y / (labelHeight + labelGap));
}

void LabelPanel::mouseDrag(const juce::MouseEvent &e) {

    const int targetIndex = labelIndexAt(e.getEventRelativeTo(this).getPosition().y);

    if (draggedIndex < 0 || targetIndex < 0 || targetIndex == draggedIndex)
        return;

    std::swap(labels[(size_t) draggedIndex], labels[(size_t) targetIndex]);
    draggedIndex = targetIndex;

    resized();
}

void LabelPanel::mouseDown(const juce::MouseEvent &e) {
    draggedIndex = labelIndexAt(e.getEventRelativeTo(this).getPosition().y);

    if (draggedIndex >= 0)
        labels[(size_t) draggedIndex]->setGrabbed(true);
}

void LabelPanel::mouseUp(const juce::MouseEvent &) {

    if (draggedIndex >= 0 && draggedIndex < (int) labels.size())
        labels[(size_t) draggedIndex]->setGrabbed(false);

    draggedIndex = -1;
}

void LabelPanel::addFileLabel(juce::String fileName)
{

    auto fileLabel = std::make_unique<FileLabel>(context);
    fileLabel->setFileName(fileName);
    fileLabel->addMouseListener(this, true);

    FileLabel* addedLabel = fileLabel.get();
    const juce::Component::SafePointer<LabelPanel> panel(this);

    fileLabel->onRemove = [panel, addedLabel] {
        juce::MessageManager::callAsync([panel, addedLabel] {
            if (panel != nullptr)
                panel->removeFileLabel(addedLabel);
        });
    };

    fileLabel->onMouseClicked = [panel, addedLabel] {
        if (panel == nullptr) {
            return;
        }

        panel->setSelectedLabel(addedLabel);

        if (panel->onLabelClicked != nullptr) {
            panel->onLabelClicked(addedLabel);
        }
    };

    addAndMakeVisible(*fileLabel);
    labels.push_back(std::move(fileLabel));

    resized();
}

void LabelPanel::setSelectedLabel(const FileLabel* label)
{
    for (auto& candidate : labels) {
        candidate->setSelected(candidate.get() == label);
    }
}

void LabelPanel::removeFileLabel(const FileLabel* label)
{
    const auto match = std::find_if(labels.begin(), labels.end(),
                                    [label](const auto& candidate) { return candidate.get() == label; });

    if (match == labels.end())
        return;

    labels.erase(match);
    draggedIndex = -1;

    resized();
}

