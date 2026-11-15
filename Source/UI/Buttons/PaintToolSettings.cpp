#include "PaintToolSettings.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"

PaintToolSettings::PaintToolSettings(NodeCanvas& nodeCanvas, juce::ValueTree colourPresets, juce::UndoManager& undoManager)
    : nodeCanvas    (nodeCanvas),
      colourSelector(colourPresets, undoManager),
      sizeField     (undoManager),
      flowField     (undoManager)
{
    ValueField& valueField = nodeCanvas.valueField;
    const float brushFlow  = juce::jlimit(minBrushFlow, maxBrushFlow, valueField.brushFlow);

    paintTool.icon = &ArsNovaUI::CustomLookAndFeel::drawBrushIcon;

    colourSelector.shape = ArsNovaUI::ColourSelector::Shape::Circle;

    configureValueFields(valueField, brushFlow);

    paintTool.setTooltip("Brush");
    colourSelector.setTooltip("Brush colour");

    componentCallBack();

    addAndMakeVisible(paintTool);
    addAndMakeVisible(colourSelector);
    addAndMakeVisible(sizeField);
    addAndMakeVisible(flowField);
}

void PaintToolSettings::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel::get(*this).drawPane(graphics, getLocalBounds().toFloat());
}

void PaintToolSettings::resized()
{
    const int contentInset = juce::roundToInt(getHeight() * ArsNovaUI::Theme::contentInsetRatio);

    auto bounds = getLocalBounds().reduced(contentInset);

    const int width      = bounds.getWidth();
    const int widthCell  = juce::roundToInt(width * cellWidthRatio);
    const int cellSide   = juce::jmin(bounds.getHeight(), widthCell);
    const int cellGap    = juce::roundToInt(width * ArsNovaUI::Theme::contentSpacingRatio);
    const int labelWidth = juce::roundToInt(width * labelWidthRatio);

    paintTool.setBounds(bounds.removeFromLeft(cellSide).withSizeKeepingCentre(cellSide, cellSide));
    bounds.removeFromLeft(cellGap);

    colourSelector.setBounds(bounds.removeFromLeft(cellSide).withSizeKeepingCentre(cellSide, cellSide));
    bounds.removeFromLeft(cellGap);

    const int partGapCount = 3;
    const int editorWidth  = (bounds.getWidth() - labelWidth * 2 - cellGap * partGapCount) / 2;
    const int fieldWidth   = labelWidth + cellGap + editorWidth;

    sizeField.labelWidth = labelWidth;
    flowField.labelWidth = labelWidth;
    sizeField.gap        = cellGap;
    flowField.gap        = cellGap;

    sizeField.setBounds(bounds.removeFromLeft(fieldWidth));
    bounds.removeFromLeft(cellGap);

    flowField.setBounds(bounds.removeFromLeft(fieldWidth));
}

void PaintToolSettings::configureValueFields(ValueField &valueField, const float brushFlow) {
    auto sizeFormat = std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, ArsNovaUI::ValueFormat::editableDecimalPlaces);
    auto flowFormat = std::make_unique<ArsNovaUI::NumberFormat>(0.0, 1.0, ArsNovaUI::ValueFormat::editableDecimalPlaces);

    sizeFormat->keepsTrailingZeros = true;
    flowFormat->keepsTrailingZeros = true;
    sizeField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    flowField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    sizeField.editor.backdrop      = ArsNovaUI::ValueEditor::Backdrop::Underline;
    flowField.editor.backdrop      = ArsNovaUI::ValueEditor::Backdrop::Underline;

    sizeField.editor.setJustification(juce::Justification::centredLeft);
    flowField.editor.setJustification(juce::Justification::centredLeft);

    sizeField.editor.setFormat(std::move(sizeFormat));
    flowField.editor.setFormat(std::move(flowFormat));

    sizeField.editor.boundValue = juce::jmap(valueField.brushRadius, minBrushRadius, maxBrushRadius, 0.0f, 1.0f);
    flowField.editor.boundValue = juce::jmap(brushFlow, minBrushFlow, maxBrushFlow, 0.0f, 1.0f);

    sizeField.label.setJustificationType(juce::Justification::centredRight);
    flowField.label.setJustificationType(juce::Justification::centredRight);

    sizeField.label.setText("SIZE", juce::dontSendNotification);
    flowField.label.setText("RATE", juce::dontSendNotification);
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
    paintTool.onClick = [this]() {
        if (nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
            nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);
            return;
        }

        nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::Paint);

        setPaintMode(paintLayer);
    };

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
