#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Editors/LabeledEditor.h"
#include "ColourSelector.h"

class CustomLookAndFeel;

class SettingsMenu : public juce::Component
{
public:

    SettingsMenu(CustomLookAndFeel& lookAndFeel, juce::PropertiesFile& interfaceSettings, juce::ValueTree colourPresets, juce::UndoManager& undoManager);
    ~SettingsMenu() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    static constexpr int   defaultWidth             = 420;
    static constexpr int   defaultHeight            = 320;
    static constexpr float paddingRatio             = 0.05f;
    static constexpr float rowHeightRatio           = 0.1f;
    static constexpr float selectorWidthRatio       = 1.6f;
    static constexpr float selectorInsetRatio       = 0.15f;
    static constexpr float settingsPickerWidthRatio = 0.55f;

private:

    CustomLookAndFeel&    lookAndFeel;
    juce::PropertiesFile& interfaceSettings;

    CaptionLabel   themeColourLabel;
    ColourSelector themeColourSelector;
};
