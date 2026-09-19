#include "BottomBar.h"
#include "../Theme/CustomLookAndFeel.h"
#include "../Canvas/NodeCanvas.h"

BottomBar::BottomBar(ApplicationContext& context)
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
    auto bounds = getContentBounds();
    int  height = bounds.getHeight();

    paintPanel->setBounds(bounds.removeFromLeft(
        juce::roundToInt(height * PaintToolSettings::widthToHeightRatio)));

    arrowButton->setBounds(bounds.removeFromRight(height));
    bounds.removeFromRight(cellGap);
    spanTool->setBounds(bounds.removeFromRight(height));
}
