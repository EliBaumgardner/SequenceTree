#include "PatchControls.h"
#include "../Theme/CustomLookAndFeel.h"

PatchControls::PatchControls(juce::AudioProcessor& processor, const juce::File& patchFolder)
    : processor(processor),
      patchFolder(patchFolder)
{
    addButton(&CustomLookAndFeel::drawImportIcon,     "Open a saved patch", [this]() { chooseFileToOpen(); });
    addButton(&CustomLookAndFeel::drawResetPatchIcon, "Reset the patch",    [this]() { confirmReset(); });
}

void PatchControls::chooseFileToOpen()
{
    const int chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser = std::make_unique<juce::FileChooser>("Open patch", patchFolder, juce::String("*") + patchExtension);

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File  chosen = chooser.getResult();
        juce::MemoryBlock state;

        if (chosen == juce::File() || ! chosen.loadFileAsData(state)) {
            return;
        }

        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
    });
}

void PatchControls::chooseFileToSave()
{
    const int chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser = std::make_unique<juce::FileChooser>("Save patch", patchFolder.getChildFile(juce::String("Untitled") + patchExtension), juce::String("*") + patchExtension);

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& chooser) {
        const juce::File  chosen = chooser.getResult();
        juce::MemoryBlock state;

        if (chosen == juce::File()) {
            return;
        }

        processor.getStateInformation(state);

        if (! chosen.withFileExtension(patchExtension).replaceWithData(state.getData(), state.getSize())) {
            juce::AlertWindow::showAsync(juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::WarningIcon).withTitle("Patch not saved")
                                             .withMessage(chosen.getFileName() + " could not be written, so the patch was not reset.").withButton("OK")
                                             .withAssociatedComponent(this), nullptr);

            return;
        }

        resetPatch();
    });
}

void PatchControls::confirmReset()
{
    const auto options = juce::MessageBoxOptions().withIconType(juce::MessageBoxIconType::QuestionIcon).withTitle("Reset patch")
                             .withMessage("Save the current patch before resetting?").withButton("Yes").withButton("No").withButton("Cancel")
                             .withAssociatedComponent(this);

    juce::AlertWindow::showAsync(options, [safeThis = juce::Component::SafePointer<PatchControls>(this)](int result) {
        if (safeThis == nullptr) {
            return;
        }

        switch (static_cast<ResetAnswer>(result)) {
            case ResetAnswer::Yes:    safeThis->chooseFileToSave(); break;
            case ResetAnswer::No:     safeThis->resetPatch();       break;
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
