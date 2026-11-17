#include "PatchControls.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "../../Graph/TypedProperties.h"
#include <ArsNovaUI/Menus/Dialog.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/StateJson.h"
#include "../../Graph/ValueTreeIdentifiers.h"

PatchControls::PatchControls(juce::AudioProcessor& processor, juce::ValueTree patch, const juce::File& patchFolder)
    : processor  (processor),
      patch      (patch),
      patchFolder(patchFolder),
      nameEditor (nameUndoManager)
{
    nameEditor.fontStyle      = ArsNovaUI::Theme::FontStyle::Regular;
    nameEditor.onEditFinished = [this]() { renamePatch(); };
    nameEditor.setFormat(std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters));
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.bindEditor(patch, ValueTreeIdentifiers::PatchName);
    nameEditor.setTooltip("Rename the patch");

    openButton.icon    = &ArsNovaUI::IconLibrary::get().chevronDown;
    openButton.onClick = [this]() { chooseFileToOpen(); };
    openButton.setTooltip("Open a saved patch");

    resetButton.icon    = &ArsNovaUI::IconLibrary::get().reload;
    resetButton.onClick = [this]() { confirmReset(); };
    resetButton.setTooltip("Reset the patch");

    addAndMakeVisible(nameEditor);
    addAndMakeVisible(openButton);
    addAndMakeVisible(resetButton);
}

void PatchControls::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const float separatorX     = resetButton.getX() - separatorGap * 0.5f - ArsNovaUI::Theme::borderThickness * 0.5f;
    const float separatorInset = resetButton.getHeight() * separatorInsetRatio;

    const auto separator = resetButton.getBounds().toFloat().reduced(0.0f, separatorInset).withX(separatorX).withWidth(ArsNovaUI::Theme::borderThickness);

    lookAndFeel.drawPane(graphics, getLocalBounds().toFloat());

    graphics.setColour(lookAndFeel.borderColour);
    graphics.fillRect(separator);
}

void PatchControls::resized()
{
    const int contentInset = juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio);
    const int nameInset    = juce::roundToInt(ArsNovaUI::Theme::fieldTextInset);

    auto contentBounds = getLocalBounds().reduced(contentInset);

    const int buttonWidth = juce::roundToInt(contentBounds.getHeight() * ArsNovaUI::Theme::buttonAspectRatio);

    resetButton.setBounds(contentBounds.removeFromRight(buttonWidth));
    contentBounds.removeFromRight(separatorGap);

    openButton.setBounds(contentBounds.removeFromRight(buttonWidth));

    contentBounds.removeFromLeft(nameInset);

    nameEditor.setBounds(contentBounds);
}

int PatchControls::idealWidth(int height) const
{
    const int inset       = juce::roundToInt(height * ArsNovaUI::Theme::contentInsetRatio);
    const int buttonWidth = juce::roundToInt((height - inset * 2) * ArsNovaUI::Theme::buttonAspectRatio);

    return inset * 2 + juce::roundToInt(ArsNovaUI::Theme::fieldTextInset) + nameWidth + buttonWidth * 2 + separatorGap;
}

void PatchControls::chooseFileToOpen()
{
    const int chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser = std::make_unique<juce::FileChooser>("Open patch", patchFolder, juce::String("*") + StateJson::patchExtension);

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File chosen = chooser.getResult();

        juce::MemoryBlock outgoingState;
        juce::MemoryBlock incomingState;

        if (chosen == juce::File()) {
            return;
        }

        processor.getStateInformation(outgoingState);

        if (! chosen.loadFileAsData(incomingState)) {
            return;
        }

        const int stateSize = static_cast<int>(incomingState.getSize());

        processor.setStateInformation(incomingState.getData(), stateSize);

        PatchProperties patchProperties(patch);

        const juce::String chosenName = chosen.getFileNameWithoutExtension();
        const juce::String chosenPath = chosen.getFullPathName();

        patchProperties.patchName.setValue(chosenName, nullptr);
        patchProperties.patchFile.setValue(chosenPath, nullptr);
    });
}

void PatchControls::renamePatch()
{
    PatchProperties patchProperties(patch);

    const juce::String currentPath = patchProperties.patchFile;
    const juce::String requested   = patchProperties.patchName;

    const juce::File current = juce::File(currentPath);

    juce::File renamed = current.getSiblingFile(requested + StateJson::patchExtension);

    if (requested.isEmpty()) {
        renamed = current;
    }
    else if (renamed.exists() && renamed != current) {
        renamed = current.getParentDirectory().getNonexistentChildFile(requested, StateJson::patchExtension, true);
    }

    if (current.existsAsFile() && ! current.moveFileTo(renamed)) {
        renamed = current;
    }

    const juce::String renamedName = renamed.getFileNameWithoutExtension();
    const juce::String renamedPath = renamed.getFullPathName();

    patchProperties.patchName.setValue(renamedName, nullptr);
    patchProperties.patchFile.setValue(renamedPath, nullptr);
}

void PatchControls::confirmReset()
{
    const PatchProperties patchProperties(patch);

    const juce::String patchName = patchProperties.patchName;

    ArsNovaUI::Dialog dialog;

    dialog.title   = "Reset patch";
    dialog.message = "Clear " + patchName + " back to an empty patch?";

    dialog.addButton("Reset", [safeThis = juce::Component::SafePointer<PatchControls>(this)]() {
        if (safeThis != nullptr) {
            safeThis->resetPatch();
        }
    });

    dialog.addButton("Cancel", nullptr);
    dialog.show(*this);
}

void PatchControls::resetPatch()
{
    processor.setStateInformation(nullptr, 0);

    for (juce::AudioProcessorParameter* parameter : processor.getParameters()) {
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
    }
}
