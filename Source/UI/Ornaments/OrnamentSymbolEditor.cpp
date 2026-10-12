#include "OrnamentSymbolEditor.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../../Graph/OrnamentLibraries.h"

OrnamentSymbolEditor::OrnamentSymbolEditor(OrnamentLibraries& ornamentLibraries, juce::UndoManager& undoManager)
    : sizeField(undoManager),
      ornamentLibraries(ornamentLibraries),
      undoManager(undoManager)
{
    IconButton& pencilButton = toolPane.addButton(&CustomLookAndFeel::drawPencilIcon, "Pencil: draw the symbol", [this]() { tool = SymbolTool::Pencil; });

    toolPane.addButton(&CustomLookAndFeel::drawEraserIcon, "Eraser: rub out what it touches", [this]() { tool = SymbolTool::Eraser; });

    toolPane.selection = ButtonPane::Selection::Exclusive;

    toolPane.setSelectedButton(&pencilButton);

    clearButton.icon = &CustomLookAndFeel::drawRemoveIcon;

    clearButton.onClick = [this]() {
        this->undoManager.beginNewTransaction();

        ornament.setProperty(ValueTreeIdentifiers::OrnamentSymbol, juce::String(), &this->undoManager);
    };

    clearButton.setTooltip("Clear symbol");

    sizeField.labelWidth = sizeLabelWidth;
    sizeField.gap        = sizeFieldGap;

    sizeField.label.setJustificationType(juce::Justification::centredRight);
    sizeField.label.setText("Size", juce::dontSendNotification);

    sizeField.editor.wheelResponse = ValueEditor::WheelResponse::StepValue;
    sizeField.editor.backdrop      = ValueEditor::Backdrop::Gauge;

    sizeField.editor.setJustification(juce::Justification::centredLeft);
    sizeField.editor.setFormat(std::make_unique<NumberFormat>(minBrushPercent, maxBrushPercent, brushDecimalPlaces));
    sizeField.editor.setTooltip("Pencil and eraser size, in percent of the symbol: Ctrl-drag or scroll to change");

    sizeField.editor.boundValue = brushWidth * percent;

    sizeField.editor.onValueChange = [this]() {
        brushWidth = static_cast<float>(sizeField.editor.boundValue.getValue()) / percent;

        repaint();
    };

    addAndMakeVisible(toolPane);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(sizeField);
}

void OrnamentSymbolEditor::paint(juce::Graphics& graphics)
{
    CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);
    const float        strokeWidth = drawingArea.getWidth() * brushWidth;
    const float        dashes[]    = { dashLength, dashLength };
    juce::Path         toolOutline;

    graphics.setColour(lookAndFeel.canvasColour);
    graphics.fillRect(getLocalBounds());

    graphics.setColour(lookAndFeel.borderStrongColour);
    graphics.drawRoundedRectangle(drawingArea, Theme::paneCornerRadius, Theme::borderThickness);

    graphics.setColour(lookAndFeel.textColour);

    lookAndFeel.drawStrokeSymbol(graphics, drawingArea, OrnamentLibraries::readSymbol(ornament.getProperty(ValueTreeIdentifiers::OrnamentSymbol).toString()));

    graphics.strokePath(stroke, juce::PathStrokeType(strokeWidth, juce::PathStrokeType::curved, juce::PathStrokeType::butt));

    if (! cursorPosition.has_value()) {
        return;
    }

    toolOutline.addEllipse(juce::Rectangle<float>(strokeWidth, strokeWidth).withCentre(*cursorPosition));

    juce::PathStrokeType(Theme::borderThickness).createDashedStroke(toolOutline, toolOutline, dashes, 2);

    graphics.setColour(lookAndFeel.textColour);
    graphics.fillPath(toolOutline);
}

void OrnamentSymbolEditor::resized()
{
    const auto  bounds = getLocalBounds().toFloat();
    const float side   = juce::jmin(bounds.getWidth(), bounds.getHeight()) * (1.0f - drawingInsetRatio * 2.0f);
    auto        topRow = getLocalBounds().reduced(toolPaneInset).removeFromTop(toolPaneHeight);

    drawingArea = bounds.withSizeKeepingCentre(side, side);

    toolPane.setBounds(topRow.removeFromLeft(toolPane.idealWidth(toolPaneHeight)));
    topRow.removeFromLeft(sizeFieldGap);

    sizeField.setBounds(topRow.removeFromLeft(sizeLabelWidth + sizeFieldGap + sizeEditorWidth));
    clearButton.setBounds(getLocalBounds().reduced(clearButtonInset).removeFromTop(clearButtonSize).removeFromRight(clearButtonSize));
}

void OrnamentSymbolEditor::mouseDown(const juce::MouseEvent& event)
{
    lastPoint      = event.position;
    cursorPosition = event.position;

    if (tool == SymbolTool::Eraser) {
        undoManager.beginNewTransaction();

        eraseAlong(event.position);

        return;
    }

    stroke.clear();
    stroke.startNewSubPath(lastPoint);

    repaint();
}

void OrnamentSymbolEditor::mouseDrag(const juce::MouseEvent& event)
{
    cursorPosition = event.position;

    if (tool == SymbolTool::Eraser) {
        eraseAlong(event.position);

        return;
    }

    if (event.position.getDistanceFrom(lastPoint) < pointSpacing) {
        return;
    }

    lastPoint = event.position;

    stroke.lineTo(lastPoint);

    repaint();
}

void OrnamentSymbolEditor::mouseUp(const juce::MouseEvent& event)
{
    juce::Path normalised = stroke;

    stroke.clear();

    repaint();

    if (! ornament.isValid() || normalised.getLength() <= 0.0f || drawingArea.isEmpty()) {
        return;
    }

    normalised.applyTransform(juce::AffineTransform::translation(-drawingArea.getX(), -drawingArea.getY()).scaled(1.0f / drawingArea.getWidth()));

    undoManager.beginNewTransaction();

    ornamentLibraries.addSymbolStroke(ornament, normalised, brushWidth, &undoManager);
}

void OrnamentSymbolEditor::mouseMove(const juce::MouseEvent& event)
{
    cursorPosition = event.position;

    repaint();
}

void OrnamentSymbolEditor::mouseExit(const juce::MouseEvent& event)
{
    cursorPosition.reset();

    repaint();
}

juce::MouseCursor OrnamentSymbolEditor::getMouseCursor()
{
    const CustomLookAndFeel& lookAndFeel = CustomLookAndFeel::get(*this);

    if (tool == SymbolTool::Eraser) {
        return lookAndFeel.eraserCursor;
    }

    return lookAndFeel.pencilCursor;
}

void OrnamentSymbolEditor::eraseAlong(juce::Point<float> target)
{
    const float distance = lastPoint.getDistanceFrom(target);
    const float radius   = drawingArea.getWidth() * brushWidth * 0.5f;
    const int   steps    = juce::jmax(1, static_cast<int>(std::ceil(distance / juce::jmax(radius * eraserStepRatio, pointSpacing))));
    const float scale    = 1.0f / drawingArea.getWidth();

    repaint();

    if (! ornament.isValid() || drawingArea.isEmpty()) {
        return;
    }

    for (int step = 1; step <= steps; ++step) {
        const auto centre = (lastPoint + (target - lastPoint) * (static_cast<float>(step) / steps) - drawingArea.getPosition()) * scale;

        ornamentLibraries.eraseSymbol(ornament, centre, brushWidth * 0.5f, &undoManager);
    }

    lastPoint = target;
}
