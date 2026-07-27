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

    arrowButton = std::make_unique<IconButton>(
        [this](juce::Graphics& g, juce::Rectangle<float> bounds, const ButtonState& state) {
            CustomLookAndFeel::get(*this).drawArrowToolIcon(g, bounds, state);
        }, applicationContext.lookAndFeel);

    arrowButton->setTooltip("Arrow Types");
    arrowButton->onClick = [this]() { arrowWindowLauncher.show(); };

    addAndMakeVisible(*arrowButton);

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

void BottomBar::showPaintSettings()
{
    PaintToolSettings::PaintSetting setting = PaintToolSettings::PaintSetting::Pitch;

    if (applicationContext.currentDisplayMode == NodeDisplayMode::Velocity) {
        setting = PaintToolSettings::PaintSetting::Velocity;
    }

    applyPaintSetting(setting);

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
    CustomLookAndFeel::get(*this).drawBar(g, getLocalBounds().toFloat(), false);
}

void BottomBar::resized()
{
    auto bounds= getLocalBounds().reduced(4);
    int  height               = bounds.getHeight();

    paintTool->setBounds(bounds.removeFromRight(height));
    bounds.removeFromRight(cellGap);
    arrowButton->setBounds(bounds.removeFromRight(height));
}
