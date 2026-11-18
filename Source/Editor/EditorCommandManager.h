#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

class EditorCommandManager : public juce::ApplicationCommandManager
{
public:

    explicit EditorCommandManager(juce::ApplicationCommandTarget& windowTarget);

    juce::ApplicationCommandTarget* getFirstCommandTarget(juce::CommandID commandID) override;

private:

    juce::ApplicationCommandTarget& windowTarget;
};
