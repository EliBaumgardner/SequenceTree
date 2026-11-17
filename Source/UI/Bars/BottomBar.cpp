#include "BottomBar.h"
#include <ArsNovaUI/Theme/IconLibrary.h>
#include "../../Graph/TypedProperties.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>
#include "../Canvas/NodeCanvas.h"

#include "../../Util/NodeInfo.h"
#include "../../Graph/OrnamentLibraries.h"
#include "../../Input/GraphDocument.h"
#include "../../Input/Gestures/NotePreviewGestures.h"
#include "../../Graph/ValueTreeIdentifiers.h"

BottomBar::BottomBar(NodeCanvas& nodeCanvas, GraphDocument& graphDocument, NotePreviewGestures& notePreviewGestures, juce::ValueTree colourPresets, OrnamentLibraries& ornamentLibraries)
    : ArsNovaUI::Bar     (nodeCanvas, Orientation::Horizontal, ArsNovaUI::Theme::contentInsetRatio),
      ornamentPane       (nodeCanvas, ornamentLibraries),
      nodeCanvas         (nodeCanvas),
      notePreviewGestures(notePreviewGestures),
      paintPanel         (nodeCanvas, colourPresets, graphDocument.undoManager),
      countsField        (graphDocument.undoManager),
      bindPane           (graphDocument.canvasSettings)
{
    auto countsFormat = std::make_unique<ArsNovaUI::NumberFormat>(NodeInfo::minimumCountLimit, NodeInfo::maximumCountLimit);

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

    countsFormat->suffix      = "x";
    countsField.backdrop      = ArsNovaUI::ValueEditor::Backdrop::Field;
    countsField.wheelResponse = ArsNovaUI::ValueEditor::WheelResponse::StepValue;
    countsField.setFormat(std::move(countsFormat));
    countsField.setTooltip("Preview count");

    countsField.boundValue.referTo(notePreviewGestures.count);

    addAndMakeVisible(paintPanel);
    addAndMakeVisible(toolPane);
    addAndMakeVisible(quaverPane);
    addAndMakeVisible(bindPane);
    addAndMakeVisible(ornamentPane);
    addAndMakeVisible(countsField);
}

void BottomBar::resized()
{
    auto bounds = getContentBounds();

    const int height          = bounds.getHeight();
    const int spacing         = contentSpacing;
    const int toolPaneWidth   = toolPane.idealWidth(height);
    const int bindPaneWidth   = bindPane.idealWidth(height);
    const int ornamentWidth   = ornamentPane.idealWidth(height);
    const int paneWidth       = quaverPaneWidth;
    const int paneInset       = juce::roundToInt(height * ArsNovaUI::Theme::contentInsetRatio);
    const int paneInsets      = paneInset * 2;
    const int innerHeight     = juce::jmax(0, height - paneInsets);
    const int buttonSlotWidth = juce::roundToInt(paneWidth * quaverButtonWidthRatio);
    const int buttonWidth     = juce::jlimit(0, innerHeight, buttonSlotWidth - paneInsets);

    const float textHeight    = ArsNovaUI::Theme::textHeight;
    const float captionHeight = textHeight * captionHeightRatio;

    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);

    const juce::Font captionFont = lookAndFeel.font(ArsNovaUI::Theme::FontStyle::SemiBold, captionHeight).withExtraKerningFactor(captionKerningFactor);

    juce::Rectangle<int> quaverPaneBounds;

    paintPanel.sizeField.label.setFont(captionFont);
    paintPanel.flowField.label.setFont(captionFont);

    paintPanel.sizeField.label.setColour(juce::Label::textColourId, lookAndFeel.dimTextColour);
    paintPanel.flowField.label.setColour(juce::Label::textColourId, lookAndFeel.dimTextColour);

    paintPanel.sizeField.editor.setFontHeight(textHeight);
    paintPanel.flowField.editor.setFontHeight(textHeight);
    countsField                .setFontHeight(textHeight);

    paintPanel.setBounds(bounds.removeFromLeft(paintPanelWidth));

    bindPane.setBounds(bounds.removeFromRight(bindPaneWidth));
    bounds.removeFromRight(spacing);

    toolPane.setBounds(bounds.removeFromRight(toolPaneWidth));
    bounds.removeFromRight(spacing);

    ornamentPane.setBounds(bounds.removeFromRight(ornamentWidth));
    bounds.removeFromRight(spacing);

    quaverPaneBounds = bounds.removeFromRight(paneWidth);
    quaverPane.gridLayout = ArsNovaUI::ButtonPane::Grid { buttonWidth, innerHeight, 0, paneInset };

    quaverPane.setBounds(quaverPaneBounds);
    quaverPane.resized();

    quaverPaneBounds = quaverPaneBounds.reduced(0, paneInset).withTrimmedRight(paneInset);

    quaverPaneBounds.removeFromLeft(buttonSlotWidth);

    countsField.setBounds(quaverPaneBounds);
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
           + quaverPaneWidth + contentSpacing * paneGapCount;
}

void BottomBar::showQuaverMenu()
{
    ArsNovaUI::ContextMenu menu;

    menu.addItem("repeat", ArsNovaUI::ContextMenu::ItemKind::Toggle, [this]() { notePreviewGestures.toggleRepeat(); }, true, notePreviewGestures.repeat);

    menu.show(*quaverTool);
}
