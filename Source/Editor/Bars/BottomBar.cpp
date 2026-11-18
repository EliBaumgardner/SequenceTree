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
    : ArsNovaUI::Bar     (nodeCanvas, Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      ornamentPane       (nodeCanvas, ornamentLibraries),
      nodeCanvas         (nodeCanvas),
      notePreviewGestures(notePreviewGestures),
      paintPanel         (nodeCanvas, colourPresets, graphDocument.undoManager),
      countField         (graphDocument.undoManager),
      bindPane           (graphDocument.canvasSettings)
{
    auto countFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit);

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
        paintPanel.paintTool.setSelected(tool == NodeCanvas::CanvasTool::Paint);
        spanTool->setSelected(tool == NodeCanvas::CanvasTool::Span);
        quaverTool->setSelected(tool == NodeCanvas::CanvasTool::NotePreview);
        ornamentPane.button->setSelected(tool == NodeCanvas::CanvasTool::Ornament);
    };

    paintPanel.paintTool.state.look = ArsNovaUI::ButtonState::Look::Raised;
    quaverTool->state.look          = ArsNovaUI::ButtonState::Look::Raised;
    spanTool->state.look            = ArsNovaUI::ButtonState::Look::Raised;

    quaverTool->onRightClick = [this]() {
        showQuaverMenu();
    };

    countFormat->suffix   = "x";
    countField.labelWidth = countLabelWidth;
    countField.gap        = countLabelGap;

    countField.label.setText("count", juce::dontSendNotification);

    countField.editor.backdrop      = ArsNovaUI::ValueEditor::Backdrop::Field;
    countField.editor.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    countField.editor.setFormat(std::move(countFormat));
    countField.editor.setTooltip("Preview count");

    countField.editor.boundValue.referTo(notePreviewGestures.count);

    addAndMakeVisible(paintPanel);
    addAndMakeVisible(toolPane);
    addAndMakeVisible(quaverPane);
    addAndMakeVisible(bindPane);
    addAndMakeVisible(ornamentPane);
}

void BottomBar::resized()
{
    auto bounds = getContentBounds();

    const int height        = bounds.getHeight();
    const int spacing       = contentSpacing;
    const int toolPaneWidth = toolPane.idealWidth(height);
    const int quaverWidth   = quaverPane.idealWidth(height);
    const int bindPaneWidth = bindPane.idealWidth(height);
    const int ornamentWidth = ornamentPane.idealWidth(height);

    const float textHeight    = ArsNovaUI::Theme::textHeight;
    const float captionHeight = textHeight * captionHeightRatio;

    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Font captionFont = lookAndFeel.font(ArsNovaUI::Theme::FontStyle::SemiBold, captionHeight).withExtraKerningFactor(captionKerningFactor);

    paintPanel.sizeField.label.setFont(captionFont);
    paintPanel.flowField.label.setFont(captionFont);

    paintPanel.sizeField.label.setColour(juce::Label::textColourId, lookAndFeel.dimTextColour);
    paintPanel.flowField.label.setColour(juce::Label::textColourId, lookAndFeel.dimTextColour);

    paintPanel.sizeField.editor.setFontHeight(textHeight);
    paintPanel.flowField.editor.setFontHeight(textHeight);

    paintPanel.setBounds(bounds.removeFromLeft(paintPanelWidth));

    bindPane.setBounds(bounds.removeFromRight(bindPaneWidth));
    bounds.removeFromRight(spacing);

    toolPane.setBounds(bounds.removeFromRight(toolPaneWidth));
    bounds.removeFromRight(spacing);

    ornamentPane.setBounds(bounds.removeFromRight(ornamentWidth));
    bounds.removeFromRight(spacing);

    quaverPane.setBounds(bounds.removeFromRight(quaverWidth));
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

int BottomBar::narrowestWidth() const
{
    const auto bounds = getContentBounds();

    const int paneGapCount = 4;
    const int height       = bounds.getHeight();

    return getWidth() - bounds.getWidth() + paintPanelWidth + bindPane.idealWidth(height) + toolPane.idealWidth(height) + ornamentPane.idealWidth(height)
           + quaverPane.idealWidth(height) + contentSpacing * paneGapCount;
}

void BottomBar::showQuaverMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addItem("repeat", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this]() { notePreviewGestures.toggleRepeat(); }, true, notePreviewGestures.repeat);

    menu.addComponent(countField, countFieldWidth, ArsNovaUI::Theme::popupMenuItemHeight);

    menu.show(*quaverTool);
}
