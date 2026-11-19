#include "PaintToolSettings.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Editor/Canvas/NodeCanvas.h"

PaintToolSettings::PaintToolSettings(NodeCanvas& nodeCanvas, juce::ValueTree colourPresets, juce::UndoManager& undoManager)
    : nodeCanvas    (nodeCanvas),
      colourSelector(colourPresets, undoManager),
      sizeField     (undoManager),
      flowField     (undoManager)
{
    ValueField& valueField = nodeCanvas.valueField;
    const float brushFlow  = juce::jlimit(minBrushFlow, maxBrushFlow, valueField.brushFlow);

    colourSelector.shape = ArsNovaUI::ColourSelector::Shape::Square;

    colourLabel.setText("Colour", juce::dontSendNotification);

    configureValueFields(valueField, brushFlow);

    colourSelector.setTooltip("Brush colour");

    componentCallBack();

    addAndMakeVisible(colourLabel);
    addAndMakeVisible(colourSelector);
    addAndMakeVisible(sizeField);
    addAndMakeVisible(flowField);
}


void PaintToolSettings::resized()
{
    const int rowHeight = ArsNovaUI::Theme::popupMenuItemHeight;
    const int textInset = juce::roundToInt(ArsNovaUI::Theme::popupMenuTextInset);

    const int rowWidth         = getWidth() - textInset * 2;
    const int labelWidth       = rowWidth - editorWidth;
    const int colourLabelWidth = rowWidth - swatchWidth;

    juce::Rectangle<int> bounds = getLocalBounds().reduced(textInset, 0);

    const juce::Rectangle<int> colourRow       = bounds.removeFromTop(rowHeight);
    const juce::Rectangle<int> sizeRow         = bounds.removeFromTop(rowHeight);
    const juce::Rectangle<int> flowRow         = bounds.removeFromTop(rowHeight);
    const juce::Rectangle<int> colourLabelArea = colourRow.withTrimmedRight(swatchWidth);
    const juce::Rectangle<int> swatchArea      = colourRow.withTrimmedLeft(colourLabelWidth);

    colourLabel.setBounds(colourLabelArea.withSizeKeepingCentre(colourLabelWidth, editorHeight));
    colourSelector.setBounds(swatchArea.withSizeKeepingCentre(swatchWidth, editorHeight));

    sizeField.labelWidth = labelWidth;
    flowField.labelWidth = labelWidth;

    sizeField.setBounds(sizeRow.withSizeKeepingCentre(rowWidth, editorHeight));
    flowField.setBounds(flowRow.withSizeKeepingCentre(rowWidth, editorHeight));
}

void PaintToolSettings::configureValueFields(ValueField &valueField, const float brushFlow) {
    auto sizeFormat = std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, ArsNovaUI::ValueFormat::editableDecimalPlaces);
    auto flowFormat = std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, ArsNovaUI::ValueFormat::editableDecimalPlaces);

    sizeFormat->keepsTrailingZeros = true;
    flowFormat->keepsTrailingZeros = true;
    sizeField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    flowField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;

    sizeField.editor.setFontHeight(ArsNovaUI::Theme::textHeight);
    flowField.editor.setFontHeight(ArsNovaUI::Theme::textHeight);

    sizeField.editor.setFormat(std::move(sizeFormat));
    flowField.editor.setFormat(std::move(flowFormat));

    sizeField.editor.boundValue = juce::jmap(valueField.brushRadius, minBrushRadius, maxBrushRadius, 0.0f, 1.0f);
    flowField.editor.boundValue = juce::jmap(brushFlow, minBrushFlow, maxBrushFlow, 0.0f, 1.0f);

    sizeField.label.setText("Size", juce::dontSendNotification);
    flowField.label.setText("Rate", juce::dontSendNotification);
}

void PaintToolSettings::setPaintMode(ValueField::PaintLayer layer)
{
    paintLayer = layer;

    const juce::Colour savedColour = paintLayerColour();

    colourSelector.colour = savedColour;

    colourSelector.repaint();

    nodeCanvas.valueField.brushColour = savedColour;

    nodeCanvas.valueField.setActivePaintLayer(layer);
}

void PaintToolSettings::componentCallBack() {
    colourSelector.onColourPicked = [this](juce::Colour colour) {
        paintLayerColour() = colour;

        nodeCanvas.valueField.brushColour = colour;

        nodeCanvas.valueField.refresh();
    };

    sizeField.editor.onValueChange = [this] {
        const float sliderValue = static_cast<float>(sizeField.editor.boundValue.getValue());
        ValueField& valueField  = nodeCanvas.valueField;

        valueField.brushRadius = juce::jmap(sliderValue, 0.0f, 1.0f, minBrushRadius, maxBrushRadius);

        valueField.updateBrushCursor();
    };

    flowField.editor.onValueChange = [this] {
        const float sliderValue = static_cast<float>(flowField.editor.boundValue.getValue());

        nodeCanvas.valueField.brushFlow = juce::jmap(sliderValue, 0.0f, 1.0f, minBrushFlow, maxBrushFlow);
    };
}

juce::Colour& PaintToolSettings::paintLayerColour()
{
    switch (paintLayer) {
        case ValueField::PaintLayer::Velocity: return velocityColour;
        case ValueField::PaintLayer::Duration: return durationColour;
        case ValueField::PaintLayer::Pitch:    return pitchColour;
    }

    return pitchColour;
}
