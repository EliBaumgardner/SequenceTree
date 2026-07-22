#include "BottomBar.h"
#include "Theme/CustomLookAndFeel.h"
#include "Canvas/NodeCanvas.h"

BottomBar::BottomBar(ApplicationContext& context)
    : applicationContext(context)
{
    setLookAndFeel(applicationContext.lookAndFeel);

    paintSettingsLauncher.createIfNeeded();

    paintTool = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawPaintToolIcon(g, bounds, state);
        }, applicationContext.lookAndFeel);

    paintTool->onClick      = [this]() { togglePaintMode(); };
    paintTool->onRightClick = [this]() { showPaintSettings(); };

    addAndMakeVisible(*paintTool);

    arrowTool = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawArrowToolIcon(g, bounds, state);
        }, applicationContext.lookAndFeel);

    arrowTool->onClick = [this]() { toggleArrowMode(); };

    addAndMakeVisible(*arrowTool);

    applicationContext.onDisplayModeChanged = [this](NodeDisplayMode mode) {
        switch (mode) {
            case NodeDisplayMode::Pitch:    applyPaintSetting(PaintToolSettings::PaintSetting::Pitch);    break;
            case NodeDisplayMode::Velocity: applyPaintSetting(PaintToolSettings::PaintSetting::Velocity); break;
            default: break;
        }
    };
}

void BottomBar::togglePaintMode()
{
    paintTool->toggleSelected();

    const bool paintMode = paintTool->isSelected();
    applicationContext.canvas->setPaintMode(paintMode);

    if (paintMode) {
        if (auto* settings = paintSettingsLauncher.getContentAs<PaintToolSettings>()) {
            settings->setPaintMode(settings->paintSetting);
        }
    }
}

void BottomBar::toggleArrowMode()
{
    arrowTool->toggleSelected();
    applicationContext.canvas->danglingArrowLayer.setArrowMode(arrowTool->isSelected());
}

void BottomBar::showPaintSettings()
{
    applyPaintSetting(applicationContext.currentDisplayMode == NodeDisplayMode::Velocity
                          ? PaintToolSettings::PaintSetting::Velocity
                          : PaintToolSettings::PaintSetting::Pitch);

    paintSettingsLauncher.show();
}

void BottomBar::applyPaintSetting(PaintToolSettings::PaintSetting setting)
{
    if (auto* settings = paintSettingsLauncher.getContentAs<PaintToolSettings>()) {
        settings->setPaintMode(setting);
    }
}

void BottomBar::paint(juce::Graphics& g)
{
    const Theme& theme = CustomLookAndFeel::get(*this);
    auto bounds = getLocalBounds().toFloat();

    juce::ColourGradient gradient(theme.barColour.darker(0.04f),   0, bounds.getY(),
                                  theme.barColour.brighter(0.06f), 0, bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRect(bounds);

    g.setColour(juce::Colours::black.withAlpha(0.35f));
    g.drawHorizontalLine((int) bounds.getY(), bounds.getX(), bounds.getRight());

    g.setColour(theme.barColour.brighter(0.12f));
    g.drawHorizontalLine((int) bounds.getBottom() - 1, bounds.getX(), bounds.getRight());
}

void BottomBar::resized()
{
    auto bounds= getLocalBounds().reduced(4);
    int  height               = bounds.getHeight();

    paintTool->setBounds(bounds.removeFromRight(height));
    bounds.removeFromRight(cellGap);
    arrowTool->setBounds(bounds.removeFromRight(height));
}
