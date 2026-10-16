#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <ArsNovaUI/Buttons/IconButton.h>
#include <ArsNovaUI/Editors/ValueEditor.h>

#include <memory>

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

    void confirmReset();
    void resetPatch  ();

    enum class ResetAnswer { Cancel = 0, Reset = 1 };

    juce::AudioProcessor& processor;
    juce::ValueTree       patch;
    juce::File            patchFolder;
    juce::UndoManager     nameUndoManager;

    ValueEditor nameEditor;
    IconButton  openButton;
    IconButton  resetButton;

    std::unique_ptr<juce::FileChooser> fileChooser;
};
