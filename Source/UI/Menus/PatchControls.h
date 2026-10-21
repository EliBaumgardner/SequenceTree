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
