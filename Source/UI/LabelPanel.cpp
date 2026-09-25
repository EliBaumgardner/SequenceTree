//
// Created by Eli Baumgardner on 8/16/26.
//

#include "LabelPanel.h"
#include "../UI/Theme/CustomLookAndFeel.h"

LabelPanel::LabelPanel(const ApplicationContext&context) : context(context) {

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
    if (labels.empty() || labelHeight <= 0) {
        return -1;
    }

    return juce::jlimit(0, (int) labels.size() - 1, y / (labelHeight + labelGap));
}

void LabelPanel::mouseDrag(const juce::MouseEvent &e) {

    const int targetIndex = labelIndexAt(e.getEventRelativeTo(this).getPosition().y);

    if (draggedIndex < 0 || targetIndex < 0 || targetIndex == draggedIndex) {
        return;
    }

    std::swap(labels[(size_t) draggedIndex], labels[(size_t) targetIndex]);

    draggedIndex = targetIndex;
    orderChanged = true;

    resized();
}

void LabelPanel::mouseDown(const juce::MouseEvent &e) {
    draggedIndex = labelIndexAt(e.getEventRelativeTo(this).getPosition().y);

    if (draggedIndex >= 0) {
        labels[(size_t) draggedIndex]->setGrabbed(true);
    }
}

void LabelPanel::mouseUp(const juce::MouseEvent &) {

    if (draggedIndex >= 0 && draggedIndex < (int) labels.size()) {
        labels[(size_t) draggedIndex]->setGrabbed(false);
    }

    draggedIndex = -1;

    if (! orderChanged) {
        return;
    }

    orderChanged = false;

    if (onLabelsReordered != nullptr) {
        std::vector<int> fileIds;
        fileIds.reserve(labels.size());

        for (const auto& label : labels) {
            fileIds.push_back(label->fileId);
        }

        onLabelsReordered(std::move(fileIds));
    }
}

void LabelPanel::applyOrder(std::span<const int> fileIds)
{
    size_t position = 0;

    for (int fileId : fileIds) {
        for (size_t candidate = position; candidate < labels.size(); ++candidate) {
            if (labels[candidate]->fileId == fileId) {
                std::swap(labels[position], labels[candidate]);
                ++position;
                break;
            }
        }
    }

    resized();
}

void LabelPanel::addFileLabel(juce::String fileName)
{

    auto fileLabel = std::make_unique<FileLabel>(context);
    fileLabel->setFileName(fileName);
    fileLabel->addMouseListener(this, true);

    const juce::Component::SafePointer<FileLabel>  addedLabel(fileLabel.get());
    const juce::Component::SafePointer<LabelPanel> panel(this);

    fileLabel->onRemove = [panel, addedLabel] {
        juce::MessageManager::callAsync([panel, addedLabel] {
            if (panel != nullptr && addedLabel != nullptr && panel->onLabelRemoved != nullptr) {
                panel->onLabelRemoved(addedLabel->fileId);
            }
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
    const auto match = std::ranges::find_if(labels,
                                            [label](const auto& candidate) { return candidate.get() == label; });

    if (match == labels.end()) {
        return;
    }

    labels.erase(match);
    draggedIndex = -1;

    resized();
}

