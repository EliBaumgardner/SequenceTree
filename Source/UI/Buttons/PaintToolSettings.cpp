#include "PaintToolSettings.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"

PaintToolSettings::PaintToolSettings(const ApplicationContext& context)
    : context(context)
{
    ValueField& valueField = context.canvas->valueField;
    const float brushFlow  = juce::jlimit(minBrushFlow, maxBrushFlow, valueField.brushFlow);

    setLookAndFeel(context.lookAndFeel);

    paintTool.icon = &CustomLookAndFeel::drawPaintToolIcon;

    paintTool.setLookAndFeel(context.lookAndFeel);

    colourSelector.shape = ColourSelector::Shape::Circle;

    configureValueFields(valueField, brushFlow);

    colourSelector.setTooltip("Brush colour");

    componentCallBack();

    addAndMakeVisible(paintTool);
    addAndMakeVisible(colourSelector);
    addAndMakeVisible(sizeField);
    addAndMakeVisible(flowField);
}

void PaintToolSettings::paint(juce::Graphics& graphics)
{
    const Theme& theme  = CustomLookAndFeel::get(*this);
    const auto   bounds = getLocalBounds().toFloat().reduced(Theme::outerButtonBoundsReduction);

    graphics.setColour(theme.buttonBarColour);
    graphics.fillRoundedRectangle(bounds, Theme::paneCornerRadius);
}

void PaintToolSettings::resized()
{
    auto      bounds     = getLocalBounds().reduced(juce::roundToInt(getHeight() * Theme::contentInsetRatio));
    const int width      = bounds.getWidth();
    const int cellSide   = juce::jmin(bounds.getHeight(), juce::roundToInt(width * cellWidthRatio));
    const int cellGap    = juce::roundToInt(width * Theme::contentSpacingRatio);
    const int labelWidth = juce::roundToInt(width * labelWidthRatio);

    paintTool.setBounds(bounds.removeFromLeft(cellSide).withSizeKeepingCentre(cellSide, cellSide));
    bounds.removeFromLeft(cellGap);

    colourSelector.setBounds(bounds.removeFromLeft(cellSide).withSizeKeepingCentre(cellSide, cellSide));
    bounds.removeFromLeft(cellGap);

    const int editorWidth = (bounds.getWidth() - labelWidth * 2 - cellGap * 3) / 2;

    sizeField.labelWidth = labelWidth;
    flowField.labelWidth = labelWidth;
    sizeField.gap        = cellGap;
    flowField.gap        = cellGap;

    sizeField.setBounds(bounds.removeFromLeft(labelWidth + cellGap + editorWidth));
    bounds.removeFromLeft(cellGap);

    flowField.setBounds(bounds.removeFromLeft(labelWidth + cellGap + editorWidth));
}

void PaintToolSettings::componentCallBack() {
    paintTool.onClick = [this]() {
        const bool paintModeEnabled = ! paintTool.state.isSelected;

        paintTool.setSelected(paintModeEnabled);

        this->context.canvas->setPaintMode(paintModeEnabled);

        if (paintModeEnabled) {
            setPaintMode(paintLayer);
        }
    };

    colourSelector.onColourPicked = [this](juce::Colour colour) {
        paintLayerColour() = colour;

        this->context.canvas->valueField.brushColour = colour;

        this->context.canvas->valueField.refresh();
    };

    sizeField.editor.onValueChange = [this] {
        const float sliderValue = static_cast<float>(sizeField.editor.boundValue.getValue());
        ValueField& valueField  = this->context.canvas->valueField;

        valueField.brushRadius = juce::jmap(sliderValue, 0.0f, 1.0f, minBrushRadius, maxBrushRadius);

        valueField.updateBrushCursor();
    };

    flowField.editor.onValueChange = [this] {
        const float sliderValue = static_cast<float>(flowField.editor.boundValue.getValue());

        this->context.canvas->valueField.brushFlow = juce::jmap(sliderValue, 0.0f, 1.0f, minBrushFlow, maxBrushFlow);
    };
}

void PaintToolSettings::configureValueFields(ValueField &valueField, const float brushFlow) {
    sizeField.editor.wheelResponse = ValueEditor::WheelResponse::StepValue;
    flowField.editor.wheelResponse = ValueEditor::WheelResponse::StepValue;

    sizeField.editor.setJustification(juce::Justification::centredLeft);
    flowField.editor.setJustification(juce::Justification::centredLeft);

    sizeField.editor.setFormat(std::make_unique<NumberFormat>(0.0, 1.0, ValueFormat::editableDecimalPlaces));
    flowField.editor.setFormat(std::make_unique<NumberFormat>(0.0, 1.0, ValueFormat::editableDecimalPlaces));

    sizeField.editor.boundValue = juce::jmap(valueField.brushRadius, minBrushRadius, maxBrushRadius, 0.0f, 1.0f);
    flowField.editor.boundValue = juce::jmap(brushFlow, minBrushFlow, maxBrushFlow, 0.0f, 1.0f);

    sizeField.label.setJustificationType(juce::Justification::centredRight);
    flowField.label.setJustificationType(juce::Justification::centredRight);

    sizeField.label.setText("Size:", juce::dontSendNotification);
    flowField.label.setText("Rate:", juce::dontSendNotification);

    sizeField.editor.setTooltip("Brush size");
    flowField.editor.setTooltip("Brush rate");
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

void PaintToolSettings::setPaintMode(ValueField::PaintLayer layer)
{
    paintLayer = layer;

    const juce::Colour savedColour = paintLayerColour();

    colourSelector.colour = savedColour;

    colourSelector.repaint();

    context.canvas->valueField.brushColour = savedColour;

    context.canvas->valueField.setActivePaintLayer(layer);
}
