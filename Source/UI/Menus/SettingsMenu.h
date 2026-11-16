#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <ArsNovaUI/Editors/LabeledEditor.h>
#include <ArsNovaUI/Menus/ColourSelector.h>
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include <ArsNovaUI/ScaledView.h>

class GraphDocument;

class SettingsMenu : public juce::Component
{
public:

    SettingsMenu(ArsNovaUI::CustomLookAndFeel& lookAndFeel, ArsNovaUI::ScaledView& scaledView, juce::ValueTree colourPresets, GraphDocument& graphDocument);

    void paint(juce::Graphics& graphics) override;
    void resized () override;

    juce::Point<int> narrowestSize() const;

    static constexpr int rowCount               = 4;
    static constexpr int horizontalPaddingCount = 3;

    static constexpr float selectorWidthRatio = 1.6f;
    static constexpr float editorWidthRatio   = 3.0f;
    static constexpr float selectorInsetRatio = 0.15f;

    static constexpr juce::Point<float> openingScale { 1.3f, 1.3f };

private:

    ArsNovaUI::CustomLookAndFeel& lookAndFeel;
    ArsNovaUI::ScaledView&        scaledView;

    ArsNovaUI::CaptionLabel themeColourLabel;
    ArsNovaUI::CaptionLabel zoomLabel;
    ArsNovaUI::CaptionLabel graphGridLabel;
    ArsNovaUI::CaptionLabel ornamentGridLabel;

    ArsNovaUI::ColourSelector themeColourSelector;

    ArsNovaUI::ValueEditor zoomEditor;
    ArsNovaUI::ValueEditor graphGridEditor;
    ArsNovaUI::ValueEditor ornamentGridEditor;
};
