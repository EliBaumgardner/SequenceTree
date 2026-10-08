#include "SettingsMenu.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeIdentifiers.h"

SettingsMenu::SettingsMenu(CustomLookAndFeel& lookAndFeel, juce::PropertiesFile& interfaceSettings, juce::ValueTree colourPresets, juce::UndoManager& undoManager)
    : lookAndFeel(lookAndFeel),
      interfaceSettings(interfaceSettings),
      themeColourSelector(colourPresets, undoManager)
{
    setLookAndFeel(&lookAndFeel);

    themeColourLabel.setText("Theme colour", juce::dontSendNotification);

    themeColourSelector.colour           = lookAndFeel.accentColour;
    themeColourSelector.pickerWidthRatio = settingsPickerWidthRatio;

    themeColourSelector.setTooltip("Colour the whole interface is tinted from");

    themeColourSelector.onColourPicked = [this](juce::Colour pickedColour) {
        juce::Desktop& desktop = juce::Desktop::getInstance();

        this->lookAndFeel.applyThemeColour(pickedColour);

        this->interfaceSettings.setValue(ValueTreeIdentifiers::ThemeColour.toString(), pickedColour.toString());

        for (int index = 0; index < desktop.getNumComponents(); ++index) {
            desktop.getComponent(index)->sendLookAndFeelChange();
        }
    };

    addAndMakeVisible(themeColourLabel);
    addAndMakeVisible(themeColourSelector);
}

SettingsMenu::~SettingsMenu()
{
    setLookAndFeel(nullptr);
}

void SettingsMenu::paint(juce::Graphics& graphics)
{
    const Theme& theme   = CustomLookAndFeel::get(*this);
    const int    padding = juce::roundToInt(getHeight() * paddingRatio);
    const auto   divider = juce::Rectangle<int>(padding, themeColourLabel.getBottom(), getWidth() - padding * 2, 1);

    graphics.fillAll(theme.surfaceColour);

    graphics.setColour(theme.borderColour);
    graphics.fillRect(divider);
}

void SettingsMenu::resized()
{
    const Theme& theme     = CustomLookAndFeel::get(*this);
    const int    padding   = juce::roundToInt(getHeight() * paddingRatio);
    const int    rowHeight = juce::roundToInt(getHeight() * rowHeightRatio);
    auto         themeRow  = getLocalBounds().reduced(padding).removeFromTop(rowHeight);
    auto         swatch    = themeRow.removeFromRight(juce::roundToInt(rowHeight * selectorWidthRatio));

    themeColourLabel.setFont(theme.font(Theme::FontStyle::Regular, rowHeight * Theme::textHeightRatio));

    themeColourLabel.setBounds(themeRow);
    themeColourSelector.setBounds(swatch.reduced(0, juce::roundToInt(rowHeight * selectorInsetRatio)));
}
