#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <ArsNovaUI/Buttons/ButtonPane.h>

#include <memory>

class PatchControls : public ButtonPane
{
public:

    PatchControls(juce::AudioProcessor& processor, const juce::File& patchFolder);

    static constexpr const char* patchExtension = ".sqtree";

private:

    void chooseFileToOpen();
    void chooseFileToSave();

    void confirmReset();
    void resetPatch  ();

    enum class ResetAnswer { Cancel = 0, Yes = 1, No = 2 };

    juce::AudioProcessor& processor;
    juce::File            patchFolder;

    std::unique_ptr<juce::FileChooser> fileChooser;
};
