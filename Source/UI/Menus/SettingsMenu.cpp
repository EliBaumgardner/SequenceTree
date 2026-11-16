#include "SettingsMenu.h"
#include <ArsNovaUI/Theme/InterfaceIdentifiers.h>
#include "../Canvas/GridMultiplier.h"
#include "../../Input/GraphDocument.h"
#include "../../Graph/ValueTreeIdentifiers.h"

SettingsMenu::SettingsMenu(ArsNovaUI::CustomLookAndFeel& lookAndFeel, ArsNovaUI::ScaledView& scaledView, juce::ValueTree colourPresets, GraphDocument& graphDocument)
    : lookAndFeel        (lookAndFeel),
      scaledView         (scaledView),
      themeColourSelector(colourPresets, graphDocument.undoManager),
      zoomEditor         (graphDocument.undoManager),
      graphGridEditor    (graphDocument.undoManager),
      ornamentGridEditor (graphDocument.undoManager)
{
    auto zoomFormat         = std::make_unique<ArsNovaUI::NumberFormat>(ArsNovaUI::ScaledView::minimumZoom, ArsNovaUI::ScaledView::maximumZoom, ArsNovaUI::ValueFormat::editableDecimalPlaces);
    auto graphGridFormat    = std::make_unique<ArsNovaUI::NumberFormat>(GridMultiplier::minimumMultiplier, GridMultiplier::maximumMultiplier, ArsNovaUI::ValueFormat::editableDecimalPlaces);
    auto ornamentGridFormat = std::make_unique<ArsNovaUI::NumberFormat>(GridMultiplier::minimumMultiplier, GridMultiplier::maximumMultiplier, ArsNovaUI::ValueFormat::editableDecimalPlaces);

    zoomFormat->suffix         = "x";
    graphGridFormat->suffix    = "x";
    ornamentGridFormat->suffix = "x";

    setLookAndFeel(&lookAndFeel);

    themeColourLabel.setText("Theme colour", juce::dontSendNotification);
    zoomLabel.setText("Interface zoom", juce::dontSendNotification);
    graphGridLabel.setText("Grid spacing", juce::dontSendNotification);
    ornamentGridLabel.setText("Ornament grid spacing", juce::dontSendNotification);

    themeColourSelector.colour  = lookAndFeel.accentColour;
    zoomEditor.backdrop         = ArsNovaUI::ValueEditor::Backdrop::Field;
    graphGridEditor.backdrop    = ArsNovaUI::ValueEditor::Backdrop::Field;
    ornamentGridEditor.backdrop = ArsNovaUI::ValueEditor::Backdrop::Field;
    zoomEditor.boundValue       = static_cast<double>(scaledView.zoom);

    zoomEditor.setFormat(std::move(zoomFormat));
    graphGridEditor.setFormat(std::move(graphGridFormat));
    ornamentGridEditor.setFormat(std::move(ornamentGridFormat));

    graphGridEditor.bindEditor(graphDocument.canvasSettings, ValueTreeIdentifiers::GraphGridMultiplier);
    ornamentGridEditor.bindEditor(graphDocument.canvasSettings, ValueTreeIdentifiers::OrnamentGridMultiplier);

    zoomEditor.onEditFinished = [this]() {
        const double enteredZoom = zoomEditor.boundValue.getValue();

        const float requestedZoom = static_cast<float>(enteredZoom);

        if (! this->scaledView.zoomFits(requestedZoom)) {
            zoomEditor.boundValue = static_cast<double>(this->scaledView.zoom);

            return;
        }

        this->scaledView.interfaceSettings.setValue(ArsNovaUI::InterfaceIdentifiers::InterfaceZoom.toString(), enteredZoom);
    };

    themeColourSelector.onColourPicked = [this](juce::Colour pickedColour) {
        juce::Desktop& desktop = juce::Desktop::getInstance();

        this->lookAndFeel.applyThemeColour(pickedColour);

        this->scaledView.interfaceSettings.setValue(ArsNovaUI::InterfaceIdentifiers::ThemeColour.toString(), pickedColour.toString());

        for (int index = 0; index < desktop.getNumComponents(); ++index) {
            desktop.getComponent(index)->sendLookAndFeelChange();
        }
    };

    addAndMakeVisible(themeColourLabel);
    addAndMakeVisible(themeColourSelector);
    addAndMakeVisible(zoomLabel);
    addAndMakeVisible(zoomEditor);
    addAndMakeVisible(graphGridLabel);
    addAndMakeVisible(graphGridEditor);
    addAndMakeVisible(ornamentGridLabel);
    addAndMakeVisible(ornamentGridEditor);
}

void SettingsMenu::paint(juce::Graphics& graphics)
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const int padding = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuSpacingRatio);

    const juce::Rectangle<int> themeDivider (padding, themeColourLabel.getBottom(), getWidth() - padding * 2, 1);
    const juce::Rectangle<int> zoomDivider  (padding, zoomLabel.getBottom(), getWidth() - padding * 2, 1);

    graphics.fillAll(theme.surfaceColour);

    graphics.setColour(theme.borderColour);
    graphics.fillRect(themeDivider);
    graphics.fillRect(zoomDivider);
}

void SettingsMenu::resized()
{
    const ArsNovaUI::Theme& theme = ArsNovaUI::CustomLookAndFeel::get(*this);

    const int padding     = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuSpacingRatio);
    const int rowHeight   = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuRowHeightRatio);
    const int rowInset      = juce::roundToInt(rowHeight * selectorInsetRatio);
    const int editorWidth   = juce::roundToInt(rowHeight * editorWidthRatio);
    const int selectorWidth = juce::roundToInt(rowHeight * selectorWidthRatio);

    const float textHeight = ArsNovaUI::Theme::textHeight;

    juce::Rectangle<int> rows            = getLocalBounds().reduced(padding);

    juce::Rectangle<int> themeRow        = rows.removeFromTop(rowHeight);
    juce::Rectangle<int> zoomRow         = rows.removeFromTop(rowHeight);
    juce::Rectangle<int> graphGridRow    = rows.removeFromTop(rowHeight);
    juce::Rectangle<int> ornamentGridRow = rows.removeFromTop(rowHeight);

    juce::Rectangle<int> swatch          = themeRow.removeFromRight(selectorWidth);
    juce::Rectangle<int> zoomBox         = zoomRow.removeFromRight(editorWidth);
    juce::Rectangle<int> graphGridBox    = graphGridRow.removeFromRight(editorWidth);
    juce::Rectangle<int> ornamentGridBox = ornamentGridRow.removeFromRight(editorWidth);

    themeColourLabel.setFont(theme.font(ArsNovaUI::Theme::FontStyle::Regular, textHeight));
    zoomLabel.setFont(theme.font(ArsNovaUI::Theme::FontStyle::Regular, textHeight));
    graphGridLabel.setFont(theme.font(ArsNovaUI::Theme::FontStyle::Regular, textHeight));
    ornamentGridLabel.setFont(theme.font(ArsNovaUI::Theme::FontStyle::Regular, textHeight));

    zoomEditor.setFontHeight(textHeight);
    graphGridEditor.setFontHeight(textHeight);
    ornamentGridEditor.setFontHeight(textHeight);

    themeColourLabel.setBounds(themeRow);
    themeColourSelector.setBounds(swatch.reduced(0, rowInset));
    zoomLabel.setBounds(zoomRow);
    zoomEditor.setBounds(zoomBox.reduced(0, rowInset));
    graphGridLabel.setBounds(graphGridRow);
    graphGridEditor.setBounds(graphGridBox.reduced(0, rowInset));
    ornamentGridLabel.setBounds(ornamentGridRow);
    ornamentGridEditor.setBounds(ornamentGridBox.reduced(0, rowInset));
}

juce::Point<int> SettingsMenu::narrowestSize() const
{
    const juce::Font textFont = lookAndFeel.font(ArsNovaUI::Theme::FontStyle::Regular, ArsNovaUI::Theme::textHeight);

    const int padding           = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuSpacingRatio);
    const int rowHeight         = juce::roundToInt(ArsNovaUI::Theme::barHeight * ArsNovaUI::Theme::menuRowHeightRatio);

    const int editorWidth       = juce::roundToInt(rowHeight * editorWidthRatio);
    const int selectorWidth     = juce::roundToInt(rowHeight * selectorWidthRatio);
    const int themeWidth        = juce::GlyphArrangement::getStringWidthInt(textFont, themeColourLabel.getText()) + selectorWidth;
    const int zoomWidth         = juce::GlyphArrangement::getStringWidthInt(textFont, zoomLabel.getText()) + editorWidth;
    const int graphGridWidth    = juce::GlyphArrangement::getStringWidthInt(textFont, graphGridLabel.getText()) + editorWidth;
    const int ornamentGridWidth = juce::GlyphArrangement::getStringWidthInt(textFont, ornamentGridLabel.getText()) + editorWidth;

    const int widestRow         = juce::jmax(themeWidth, zoomWidth, graphGridWidth, ornamentGridWidth);
    const int contentWidth      = padding * horizontalPaddingCount + widestRow;
    const int contentHeight     = padding * 2 + rowHeight * rowCount;

    return { contentWidth, contentHeight };
}
