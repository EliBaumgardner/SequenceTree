#include "BottomBar.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"
#include "../../Util/NodeInfo.h"

BottomBar::BottomBar(const ApplicationContext& context)
    : Bar(context, { Orientation::horizontal })
{
    paintPanel = std::make_unique<PaintToolSettings>(applicationContext);

    addAndMakeVisible(*paintPanel);

    arrowButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawArrowToolIcon(g, bounds, state);
        }, applicationContext.lookAndFeel);

    arrowButton->setTooltip("Arrow Types");
    arrowButton->onClick = [this]() { arrowWindowLauncher.show(); };

    addAndMakeVisible(*arrowButton);

    spanTool = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawSpanToolIcon(g, bounds, state);
        }, applicationContext.lookAndFeel);

    spanTool->setTooltip("Node Span");

    spanTool->onClick = [this]() {
        spanTool->toggleSelected();
        applicationContext.canvas->setSpanMode(spanTool->isSelected());
    };

    addAndMakeVisible(*spanTool);

    quaverTool = &quaverPane.addButton(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawQuaverToolIcon(g, bounds, state);
        },
        "Note",
        [this]() {
            quaverTool->toggleSelected();

            if (quaverTool->isSelected()) {
                applicationContext.canvas->setQuaverMode(NodeCanvas::QuaverMode::PlacingNotes);
                return;
            }

            applicationContext.canvas->setQuaverMode(NodeCanvas::QuaverMode::Off);
        });

    countsLabel.setText("counts:", juce::dontSendNotification);
    countsLabel.setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    countsLabel.setJustificationType(juce::Justification::centredLeft);
    countsLabel.setBorderSize({});
    countsLabel.setMinimumHorizontalScale(1.0f);

    countsEditor.setFormat(std::make_unique<NumberFormat>(minimumCountLimit, maximumCountLimit));
    countsEditor.wheelResponse = ValueEditor::WheelResponse::StepValue;
    countsEditor.boundValue    = minimumCountLimit;
    countsEditor.setTooltip("Counts");

    addAndMakeVisible(quaverPane);
    addAndMakeVisible(countsLabel);
    addAndMakeVisible(countsEditor);
}

void BottomBar::applyDisplayMode(NodeDisplayMode mode)
{
    switch (mode) {
        case NodeDisplayMode::Pitch:    paintPanel->setPaintMode(PaintToolSettings::PaintSetting::Pitch);    break;
        case NodeDisplayMode::Velocity: paintPanel->setPaintMode(PaintToolSettings::PaintSetting::Velocity); break;
        default: break;
    }
}

void BottomBar::resized()
{
    auto  bounds          = getContentBounds();
    int   width           = bounds.getWidth();
    int   height          = bounds.getHeight();
    int   spacing         = juce::roundToInt(width * Theme::contentSpacingRatio);
    int   toolSide        = juce::jmin(height, juce::roundToInt(width * toolWidthRatio));
    int   paneWidth       = juce::roundToInt(width * quaverPaneWidthRatio);
    int   paneInset       = juce::roundToInt(height * Theme::contentInsetRatio);
    int   innerHeight     = juce::jmax(0, height - paneInset * 2);
    int   buttonSlotWidth = juce::roundToInt(paneWidth * quaverButtonWidthRatio);
    int   buttonWidth     = juce::jlimit(0, innerHeight, buttonSlotWidth - paneInset * 2);
    int   labelWidth      = juce::roundToInt(paneWidth * countsLabelWidthRatio);
    int   editorWidth     = juce::roundToInt(paneWidth * countsEditorWidthRatio);
    float textHeight      = CustomLookAndFeel::get(*this).textHeight;
    juce::Font textFont   { juce::FontOptions(textHeight) };
    juce::Rectangle<int> quaverPaneBounds;

    paintPanel->sizeLabel.setFont(textFont);
    paintPanel->flowLabel.setFont(textFont);
    paintPanel->sizeEditor->setFontHeight(textHeight);
    paintPanel->flowEditor->setFontHeight(textHeight);
    countsLabel.setFont(textFont);
    countsEditor.setFontHeight(textHeight);

    paintPanel->setBounds(bounds.removeFromLeft(juce::roundToInt(width * paintPanelWidthRatio)));

    arrowButton->setBounds(bounds.removeFromRight(toolSide).withSizeKeepingCentre(toolSide, toolSide));
    bounds.removeFromRight(spacing);
    spanTool->setBounds(bounds.removeFromRight(toolSide).withSizeKeepingCentre(toolSide, toolSide));
    bounds.removeFromRight(spacing);

    quaverPaneBounds = bounds.removeFromRight(paneWidth);
    quaverPane.useGridLayout({ buttonWidth, innerHeight, 0, paneInset });
    quaverPane.setBounds(quaverPaneBounds);

    quaverPaneBounds = quaverPaneBounds.reduced(0, paneInset);
    quaverPaneBounds.removeFromLeft(buttonSlotWidth);
    countsLabel.setBounds(quaverPaneBounds.removeFromLeft(labelWidth));
    countsEditor.setBounds(quaverPaneBounds.removeFromLeft(editorWidth));
}
