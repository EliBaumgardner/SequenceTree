#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../Editors/LabeledEditor.h"
#include "ColourSelector.h"

class CustomLookAndFeel;
class ScaledView;

class SettingsMenu : public juce::Component
{
public:

    SettingsMenu(CustomLookAndFeel& lookAndFeel, juce::PropertiesFile& interfaceSettings, ScaledView& scaledView, juce::ValueTree colourPresets,
                 juce::UndoManager& undoManager);

    ~SettingsMenu() override;

    void paint(juce::Graphics& graphics) override;
    void resized() override;

    juce::Point<int> narrowestSize() const;

    static constexpr float selectorWidthRatio       = 1.6f;
    static constexpr float zoomEditorWidthRatio     = 3.0f;
    static constexpr float selectorInsetRatio       = 0.15f;
    static constexpr float settingsPickerWidthRatio = 0.55f;

    static constexpr juce::Point<float> openingScale { 1.3f, 1.3f };

private:

    CustomLookAndFeel&    lookAndFeel;
    juce::PropertiesFile& interfaceSettings;
    ScaledView&           scaledView;

    CaptionLabel   themeColourLabel;
    ColourSelector themeColourSelector;
    CaptionLabel   zoomLabel;
    ValueEditor    zoomEditor;
};
