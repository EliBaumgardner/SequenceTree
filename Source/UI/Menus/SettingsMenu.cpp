#include "SettingsMenu.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../../Graph/ValueTreeIdentifiers.h"
#include "../ScaledView.h"

SettingsMenu::SettingsMenu(CustomLookAndFeel& lookAndFeel, juce::PropertiesFile& interfaceSettings, ScaledView& scaledView, juce::ValueTree colourPresets,
                           juce::UndoManager& undoManager)
    : lookAndFeel(lookAndFeel),
      interfaceSettings(interfaceSettings),
      scaledView(scaledView),
      themeColourSelector(colourPresets, undoManager),
      zoomEditor(undoManager)
{
    auto zoomFormat = std::make_unique<NumberFormat>(ScaledView::minimumZoom, ScaledView::maximumZoom, ValueFormat::editableDecimalPlaces);

    zoomFormat->suffix = "x";

    setLookAndFeel(&lookAndFeel);

    themeColourLabel.setText("Theme colour", juce::dontSendNotification);
    zoomLabel.setText("Interface zoom", juce::dontSendNotification);

    themeColourSelector.colour           = lookAndFeel.accentColour;
    themeColourSelector.pickerWidthRatio = settingsPickerWidthRatio;
    zoomEditor.backdrop                  = ValueEditor::Backdrop::Field;

    themeColourSelector.setTooltip("Colour the whole interface is tinted from");
    zoomEditor.setTooltip("Size of every button, label and bar, from 1x to 2x");

    zoomEditor.setFormat(std::move(zoomFormat));

    zoomEditor.boundValue = static_cast<double>(scaledView.zoom);

    zoomEditor.onEditFinished = [this]() {
        const double enteredZoom = zoomEditor.boundValue.getValue();

        if (! this->scaledView.zoomFits(static_cast<float>(enteredZoom))) {
            zoomEditor.boundValue = static_cast<double>(this->scaledView.zoom);
            return;
        }

        this->interfaceSettings.setValue(ValueTreeIdentifiers::InterfaceZoom.toString(), enteredZoom);
    };

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
    addAndMakeVisible(zoomLabel);
    addAndMakeVisible(zoomEditor);
}

SettingsMenu::~SettingsMenu()
{
    setLookAndFeel(nullptr);
}

void SettingsMenu::paint(juce::Graphics& graphics)
{
    const Theme& theme   = CustomLookAndFeel::get(*this);
    const int    padding = juce::roundToInt(Theme::barHeight * Theme::menuSpacingRatio);
    const auto   divider = juce::Rectangle<int>(padding, themeColourLabel.getBottom(), getWidth() - padding * 2, 1);

    graphics.fillAll(theme.surfaceColour);

    graphics.setColour(theme.borderColour);
    graphics.fillRect(divider);
}

void SettingsMenu::resized()
{
    const Theme& theme      = CustomLookAndFeel::get(*this);
    const int    padding    = juce::roundToInt(Theme::barHeight * Theme::menuSpacingRatio);
    const int    rowHeight  = juce::roundToInt(Theme::barHeight * Theme::menuRowHeightRatio);
    const int    rowInset   = juce::roundToInt(rowHeight * selectorInsetRatio);
    const float  textHeight = Theme::textHeight;
    auto         rows       = getLocalBounds().reduced(padding);
    auto         themeRow   = rows.removeFromTop(rowHeight);
    auto         zoomRow    = rows.removeFromTop(rowHeight);
    auto         swatch     = themeRow.removeFromRight(juce::roundToInt(rowHeight * selectorWidthRatio));
    auto         zoomBox    = zoomRow.removeFromRight(juce::roundToInt(rowHeight * zoomEditorWidthRatio));

    themeColourLabel.setFont(theme.font(Theme::FontStyle::Regular, textHeight));
    zoomLabel.setFont(theme.font(Theme::FontStyle::Regular, textHeight));

    zoomEditor.setFontHeight(textHeight);

    themeColourLabel.setBounds(themeRow);
    themeColourSelector.setBounds(swatch.reduced(0, rowInset));
    zoomLabel.setBounds(zoomRow);
    zoomEditor.setBounds(zoomBox.reduced(0, rowInset));
}

juce::Point<int> SettingsMenu::narrowestSize() const
{
    const int        padding    = juce::roundToInt(Theme::barHeight * Theme::menuSpacingRatio);
    const int        rowHeight  = juce::roundToInt(Theme::barHeight * Theme::menuRowHeightRatio);
    const juce::Font textFont   = lookAndFeel.font(Theme::FontStyle::Regular, Theme::textHeight);
    const int        themeWidth = juce::GlyphArrangement::getStringWidthInt(textFont, themeColourLabel.getText()) + juce::roundToInt(rowHeight * selectorWidthRatio);
    const int        zoomWidth  = juce::GlyphArrangement::getStringWidthInt(textFont, zoomLabel.getText()) + juce::roundToInt(rowHeight * zoomEditorWidthRatio);

    return { padding * 3 + juce::jmax(themeWidth, zoomWidth), padding * 2 + rowHeight * 2 };
}
