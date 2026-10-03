#include "BottomBar.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Plugin/PluginProcessor.h"
#include "../../Util/NodeInfo.h"

BottomBar::BottomBar(const ApplicationContext& context)
    : Bar(context, { Orientation::Horizontal })
{
    quaverTool = &quaverPane.addButton(&CustomLookAndFeel::drawQuaverToolIcon, "Note",
        [this]() {
            quaverTool->setSelected(! quaverTool->state.isSelected);

            if (quaverTool->state.isSelected) {
                applicationContext.canvas->setQuaverMode(NodeCanvas::QuaverMode::Preview);
                return;
            }

            applicationContext.canvas->setQuaverMode(NodeCanvas::QuaverMode::Off);
        });

    spanTool = &toolPane.addButton(&CustomLookAndFeel::drawSpanToolIcon, "Node Span",
        [this]() {
            spanTool->setSelected(! spanTool->state.isSelected);

            applicationContext.canvas->setSpanMode(spanTool->state.isSelected);
        });

    toolPane.addButton(&CustomLookAndFeel::drawArrowToolIcon, "Arrow Types",
        [this]() { arrowWindowLauncher.show(); });

    countsField.editor.setTooltip("Counts");

    quaverTool->onRightClick = [this]() {
        showQuaverMenu();
    };

    countsField.label.setText("counts", juce::dontSendNotification);

    countsField.editor.setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));

    countsField.editor.wheelResponse = ValueEditor::WheelResponse::StepValue;

    countsField.editor.boundValue.referTo(applicationContext.canvas->quaverCount);

    addAndMakeVisible(paintPanel);
    addAndMakeVisible(toolPane);
    addAndMakeVisible(quaverPane);
    addAndMakeVisible(countsField);
}

void BottomBar::resized()
{
    auto                 bounds          = getContentBounds();
    const int            width           = bounds.getWidth();
    const int            height          = bounds.getHeight();
    const int            spacing         = juce::roundToInt(width * Theme::contentSpacingRatio);
    const int            toolPaneWidth   = toolPane.idealWidth(height);
    const int            paneWidth       = juce::roundToInt(width * quaverPaneWidthRatio);
    const int            paneInset       = juce::roundToInt(height * Theme::contentInsetRatio);
    const int            innerHeight     = juce::jmax(0, height - paneInset * 2);
    const int            buttonSlotWidth = juce::roundToInt(paneWidth * quaverButtonWidthRatio);
    const int            buttonWidth     = juce::jlimit(0, innerHeight, buttonSlotWidth - paneInset * 2);
    const int            labelWidth      = juce::roundToInt(paneWidth * countsLabelWidthRatio);
    const int            editorWidth     = juce::roundToInt(paneWidth * countsEditorWidthRatio);
    const float          textHeight      = CustomLookAndFeel::get(*this).textHeight;
    const juce::Font     textFont        = CustomLookAndFeel::get(*this).font(Theme::FontStyle::Regular, textHeight);
    juce::Rectangle<int> quaverPaneBounds;

    paintPanel.sizeField.label.setFont(textFont);
    paintPanel.flowField.label.setFont(textFont);
    countsField.label.setFont(textFont);

    paintPanel.sizeField.editor.setFontHeight(textHeight);
    paintPanel.flowField.editor.setFontHeight(textHeight);
    countsField.editor.setFontHeight(textHeight);

    paintPanel.setBounds(bounds.removeFromLeft(juce::roundToInt(width * paintPanelWidthRatio)));

    toolPane.setBounds(bounds.removeFromRight(toolPaneWidth));
    bounds.removeFromRight(spacing);

    quaverPaneBounds = bounds.removeFromRight(paneWidth);
    quaverPane.gridLayout = ButtonPane::Grid { buttonWidth, innerHeight, 0, paneInset };

    quaverPane.setBounds(quaverPaneBounds);
    quaverPane.resized();

    quaverPaneBounds = quaverPaneBounds.reduced(0, paneInset);

    quaverPaneBounds.removeFromLeft(buttonSlotWidth);

    countsField.labelWidth = labelWidth;
    countsField.gap        = paneInset;

    countsField.setBounds(quaverPaneBounds.removeFromLeft(labelWidth + editorWidth));
}

void BottomBar::showQuaverMenu()
{
    ContextMenu menu(applicationContext);

    menu.addItem("repeat", ContextMenu::ItemKind::Toggle, [this]() {
        applicationContext.canvas->quaverRepeat = ! applicationContext.canvas->quaverRepeat;

        applicationContext.processor->traversalSession.previewRequests.push({
            .kind   = RTPreviewRequest::Kind::SetRepeat,
            .repeat = applicationContext.canvas->quaverRepeat
        });
    }, true, applicationContext.canvas->quaverRepeat);

    menu.show(*quaverTool);
}

void BottomBar::applyDisplayMode(NodeDisplayMode mode)
{
    switch (mode) {
        case NodeDisplayMode::Pitch:    paintPanel.setPaintMode(ValueField::PaintLayer::Pitch);    break;
        case NodeDisplayMode::Velocity: paintPanel.setPaintMode(ValueField::PaintLayer::Velocity); break;
        default: break;
    }
}
