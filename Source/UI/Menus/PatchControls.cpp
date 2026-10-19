#include "PatchControls.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Plugin/PluginProcessor.h"

PatchControls::PatchControls(juce::AudioProcessor& processor, juce::ValueTree patch, const juce::File& patchFolder)
    : processor(processor),
      patch(patch),
      patchFolder(patchFolder),
      nameEditor(nameUndoManager)
{
    nameEditor.fontStyle = ArsNovaUI::Theme::FontStyle::Regular;
    openButton.icon      = &ArsNovaUI::CustomLookAndFeel::drawChevronDownIcon;
    resetButton.icon     = &ArsNovaUI::CustomLookAndFeel::drawReloadIcon;

    nameEditor.setFormat(std::make_unique<ArsNovaUI::TextFormat>(ArsNovaUI::TextFormat::labelTextLength, ArsNovaUI::TextFormat::labelCharacters));
    nameEditor.setJustification(juce::Justification::centredLeft);
    nameEditor.bindEditor(patch, ValueTreeIdentifiers::PatchName);

    nameEditor.setTooltip("Rename the patch");
    openButton.setTooltip("Open a saved patch");
    resetButton.setTooltip("Reset the patch");

    nameEditor.onEditFinished = [this]() { renamePatch(); };
    openButton.onClick        = [this]() { chooseFileToOpen(); };
    resetButton.onClick       = [this]() { confirmReset(); };

    addAndMakeVisible(nameEditor);
    addAndMakeVisible(openButton);
    addAndMakeVisible(resetButton);
}

void PatchControls::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawPane(graphics, getLocalBounds().toFloat());
}

void PatchControls::resized()
{
    auto      contentBounds = getLocalBounds().reduced(juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio));
    const int buttonWidth   = juce::roundToInt(contentBounds.getHeight() * ArsNovaUI::Theme::buttonAspectRatio);

    resetButton.setBounds(contentBounds.removeFromRight(buttonWidth));
    contentBounds.removeFromRight(ArsNovaUI::Theme::buttonGap);

    openButton.setBounds(contentBounds.removeFromRight(buttonWidth));

    nameEditor.setBounds(contentBounds);
}

int PatchControls::idealWidth(int height) const
{
    const int inset       = juce::roundToInt(height * ArsNovaUI::Theme::contentInsetRatio);
    const int buttonWidth = juce::roundToInt((height - inset * 2) * ArsNovaUI::Theme::buttonAspectRatio);

    return inset * 2 + nameWidth + buttonWidth * 2 + ArsNovaUI::Theme::buttonGap;
}

void PatchControls::chooseFileToOpen()
{
    const int chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser = std::make_unique<juce::FileChooser>("Open patch", patchFolder, juce::String("*") + SequenceTreeAudioProcessor::patchExtension);

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File  chosen = chooser.getResult();
        juce::MemoryBlock outgoingState;
        juce::MemoryBlock incomingState;

        if (chosen == juce::File()) {
            return;
        }

        processor.getStateInformation(outgoingState);

        if (! chosen.loadFileAsData(incomingState)) {
            return;
        }

        processor.setStateInformation(incomingState.getData(), static_cast<int>(incomingState.getSize()));

        patch.setProperty(ValueTreeIdentifiers::PatchName, chosen.getFileNameWithoutExtension(), nullptr);
        patch.setProperty(ValueTreeIdentifiers::PatchFile, chosen.getFullPathName(), nullptr);
    });
}

void PatchControls::renamePatch()
{
    const juce::File   current   = juce::File(patch[ValueTreeIdentifiers::PatchFile].toString());
    const juce::String requested = patch[ValueTreeIdentifiers::PatchName].toString();
    juce::File         renamed   = current.getSiblingFile(requested + SequenceTreeAudioProcessor::patchExtension);

    if (requested.isEmpty()) {
        renamed = current;
    }
    else if (renamed.exists() && renamed != current) {
        renamed = current.getParentDirectory().getNonexistentChildFile(requested, SequenceTreeAudioProcessor::patchExtension, true);
    }

    if (current.existsAsFile() && ! current.moveFileTo(renamed)) {
        renamed = current;
    }

    patch.setProperty(ValueTreeIdentifiers::PatchName, renamed.getFileNameWithoutExtension(), nullptr);
    patch.setProperty(ValueTreeIdentifiers::PatchFile, renamed.getFullPathName(), nullptr);
}

void PatchControls::confirmReset()
{
    const juce::String patchName = patch[ValueTreeIdentifiers::PatchName].toString();
    const auto         options   = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::QuestionIcon).withTitle("Reset patch")
                                       .withMessage("Clear " + patchName + " back to an empty patch?").withButton("Reset").withButton("Cancel")
                                       .withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [safeThis = juce::Component::SafePointer<PatchControls>(this)](int result) {
        if (safeThis == nullptr) {
            return;
        }

        switch (static_cast<ResetAnswer>(result)) {
            case ResetAnswer::Reset:  safeThis->resetPatch(); break;
            case ResetAnswer::Cancel: break;
        }
    });
}

void PatchControls::resetPatch()
{
    processor.setStateInformation(nullptr, 0);

    for (juce::AudioProcessorParameter* parameter : processor.getParameters()) {
        parameter->setValueNotifyingHost(parameter->getDefaultValue());
    }
}
