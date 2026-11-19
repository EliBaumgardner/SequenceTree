#include "BottomBar.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "Processor/Document/TypedProperties.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "Editor/Canvas/NodeCanvas.h"

#include "Core/Model/NodeInfo.h"
#include "Processor/Document/Ornaments/OrnamentLibraries.h"
#include "Editor/Canvas/GraphDocument.h"
#include "Editor/Canvas/NotePreviewGestures.h"
#include "Processor/Document/ValueTreeIdentifiers.h"

BottomBar::BottomBar(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, juce::ValueTree colourPresets, OrnamentLibraries& ornamentLibraries)
    : ArsNovaUI::Bar     (nodeCanvas, Orientation::Horizontal, 0.0f),
      ornamentPane       (nodeCanvas, ornamentLibraries),
      nodeCanvas         (nodeCanvas),
      notePreviewGestures(notePreviewGestures),
      paintPanel         (nodeCanvas, colourPresets, graphDocument.undoManager),
      countField         (graphDocument.undoManager),
      bindPane           (graphDocument.canvasSettings)
{
    paintTool = &paintPane.addButton(ArsNovaUI::IconLibrary::get().brush, "Brush",
        [this]() {
            if (this->nodeCanvas.activeTool == NodeCanvas::CanvasTool::Paint) {
                this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);

                return;
            }

            this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::Paint);

            paintPanel.setPaintMode(paintPanel.paintLayer);
        });

    quaverTool = &quaverPane.addButton(ArsNovaUI::IconLibrary::get().quaver, "Note",
        [this]() {
            if (this->nodeCanvas.activeTool == NodeCanvas::CanvasTool::NotePreview) {
                this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);
                return;
            }

            this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::NotePreview);
        });

    spanTool = &toolPane.addButton(ArsNovaUI::IconLibrary::get().dashedBox, "Node Span",
        [this]() {
            if (this->nodeCanvas.activeTool == NodeCanvas::CanvasTool::Span) {
                this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::None);
                return;
            }

            this->nodeCanvas.setActiveTool(NodeCanvas::CanvasTool::Span);
        });

    nodeCanvas.onActiveToolChanged = [this](NodeCanvas::CanvasTool tool) {
        paintTool->setSelected(tool == NodeCanvas::CanvasTool::Paint);
        spanTool->setSelected(tool == NodeCanvas::CanvasTool::Span);
        quaverTool->setSelected(tool == NodeCanvas::CanvasTool::NotePreview);
        ornamentPane.button->setSelected(tool == NodeCanvas::CanvasTool::Ornament);
    };

    paintTool->state.look  = ArsNovaUI::ButtonState::Look::Raised;
    quaverTool->state.look = ArsNovaUI::ButtonState::Look::Raised;
    spanTool->state.look   = ArsNovaUI::ButtonState::Look::Raised;

    paintTool->onRightClick = [this]() {
        showPaintMenu();
    };

    quaverTool->onRightClick = [this]() {
        showQuaverMenu();
    };

    configureCountField();

    paintPane.look          = ArsNovaUI::ButtonPane::Look::Flat;
    toolPane.look           = ArsNovaUI::ButtonPane::Look::Flat;
    quaverPane.look         = ArsNovaUI::ButtonPane::Look::Flat;
    bindPane.look           = ArsNovaUI::ButtonPane::Look::Flat;
    ornamentPane.look       = ArsNovaUI::ButtonPane::Look::Flat;
    paintPane.insetRatio    = toolInsetRatio;
    toolPane.insetRatio     = toolInsetRatio;
    quaverPane.insetRatio   = toolInsetRatio;
    ornamentPane.insetRatio = toolInsetRatio;

    addAndMakeVisible(paintPane);
    addAndMakeVisible(toolPane);
    addAndMakeVisible(quaverPane);
    addAndMakeVisible(bindPane);
    addAndMakeVisible(ornamentPane);
}

void BottomBar::resized()
{
    const ArsNovaUI::Bar::Layout layout = makeLayout();

    placeLayout(layout);
}

void BottomBar::applyDisplayMode(NodeInfo::DisplayMode mode)
{
    switch (mode) {
        case NodeInfo::DisplayMode::Pitch:    paintPanel.setPaintMode(ValueField::PaintLayer::Pitch);    break;
        case NodeInfo::DisplayMode::Velocity: paintPanel.setPaintMode(ValueField::PaintLayer::Velocity); break;

        case NodeInfo::DisplayMode::Duration:
        case NodeInfo::DisplayMode::CountLimit:
        case NodeInfo::DisplayMode::Channel:
        case NodeInfo::DisplayMode::RepeatValue:
        case NodeInfo::DisplayMode::Probability:
            break;
    }
}

int BottomBar::narrowestWidth()
{
    const ArsNovaUI::Bar::Layout layout = makeLayout();

    return layoutWidth(layout);
}

ArsNovaUI::Bar::Layout BottomBar::makeLayout()
{
    const int height        = getContentBounds().getHeight();
    const int paintWidth    = paintPane.idealWidth(height);
    const int quaverWidth   = quaverPane.idealWidth(height);
    const int ornamentWidth = ornamentPane.idealWidth(height);
    const int toolWidth     = toolPane.idealWidth(height);
    const int bindWidth     = bindPane.idealWidth(height);

    const ArsNovaUI::Bar::Group paintGroup { { &paintPane, paintWidth } };
    const ArsNovaUI::Bar::Group toolGroup  { { &quaverPane, quaverWidth, unrelatedItemSpace }, { &ornamentPane, ornamentWidth, unrelatedItemSpace }, { &toolPane, toolWidth } };
    const ArsNovaUI::Bar::Group bindGroup  { { &bindPane, bindWidth } };

    return { { paintGroup }, { toolGroup, bindGroup } };
}

void BottomBar::configureCountField()
{
    auto countFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit);

    countFormat->suffix   = "x";
    countField.labelWidth = countLabelWidth;
    countField.gap        = countLabelGap;

    countField.label.setText("count", juce::dontSendNotification);

    countField.editor.backdrop      = ArsNovaUI::ValueEditor::Backdrop::Field;
    countField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    countField.editor.setFormat(std::move(countFormat));
    countField.editor.setTooltip("Preview count");

    countField.editor.boundValue.referTo(notePreviewGestures.count);
}

void BottomBar::showPaintMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addComponent(paintPanel, PaintToolSettings::menuWidth, PaintToolSettings::menuHeight);

    menu.show(*paintTool);
}

void BottomBar::showQuaverMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addItem("repeat", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this]() { notePreviewGestures.toggleRepeat(); }, true, notePreviewGestures.repeat);

    menu.addComponent(countField, countFieldWidth, ArsNovaUI::Theme::popupMenuItemHeight);

    menu.show(*quaverTool);
}
