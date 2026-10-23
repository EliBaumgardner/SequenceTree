#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

#include <memory>

/*
    The controls for the patch, the saved state of the plugin kept in a file: an editor showing
    the patch's name, a button that opens another patch file, and a button that resets the patch.
    The name and the file path are the PatchName and PatchFile properties of the patch tree.

    Editing the name renames the patch file on disk, adding a number if the new name is taken, and
    stores the name the file ended up with. Opening a file asks for one in the patch folder,
    loads it with the processor's setStateInformation and records its name and path. Resetting
    asks for confirmation, then loads an empty state and returns every parameter to its default.

    Titlebar owns it and asks idealWidth how wide it needs to be. It acts on the processor from
    the message thread.
*/

class PatchControls : public juce::Component
{
public:

    PatchControls(juce::AudioProcessor& processor, juce::ValueTree patch, const juce::File& patchFolder);

    void paint  (juce::Graphics& graphics) override;
    void resized() override;

    int idealWidth(int height) const;

    static constexpr int nameWidth = 100;

private:

    void chooseFileToOpen();
    void renamePatch     ();
    void resetPatch      ();
    void confirmReset    ();

    enum class ResetAnswer { Cancel = 0, Reset = 1 };

    static constexpr int   separatorGap        = 7;
    static constexpr float separatorInsetRatio = 0.2f;

    juce::AudioProcessor& processor;
    juce::ValueTree       patch;
    juce::File            patchFolder;
    juce::UndoManager     nameUndoManager;

    ArsNovaUI::ValueEditor nameEditor;
    ArsNovaUI::IconButton  openButton;
    ArsNovaUI::IconButton  resetButton;

    std::unique_ptr<juce::FileChooser> fileChooser;
};
