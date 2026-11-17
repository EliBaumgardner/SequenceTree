#include "CanvasFrame.h"
#include "NodeCanvas.h"
#include <ArsNovaUI/Theme/CustomLookAndFeel.h>

CanvasFrame::CanvasFrame(NodeCanvas& nodeCanvas, juce::Component& menuArea, juce::Component& titleBar, juce::Component& bottomBar)
    : nodeCanvas(nodeCanvas),
      menuArea  (menuArea),
      titleBar  (titleBar),
      bottomBar (bottomBar)
{
    setInterceptsMouseClicks(false, false);
}

void CanvasFrame::paint(juce::Graphics& graphics)
{
    ArsNovaUI::CustomLookAndFeel& lookAndFeel = ArsNovaUI::CustomLookAndFeel::get(*this);
    const float                   halfBorder  = ArsNovaUI::Theme::borderThickness * 0.5f;
    const auto                    frame       = canvasArea.toFloat().reduced(halfBorder);
    juce::Path                    surround;

    surround.addRectangle(getLocalBounds().toFloat());
    surround.addRoundedRectangle(frame, ArsNovaUI::Theme::canvasCornerRadius);
    surround.setUsingNonZeroWinding(false);

    graphics.saveState();

    const juce::Rectangle<int> menuAreaBounds  = menuArea.getBounds() - getPosition();
    const juce::Rectangle<int> titleBarBounds  = titleBar.getBounds() - getPosition();
    const juce::Rectangle<int> bottomBarBounds = bottomBar.getBounds() - getPosition();

    graphics.excludeClipRegion(menuAreaBounds);
    graphics.excludeClipRegion(titleBarBounds);
    graphics.excludeClipRegion(bottomBarBounds);
    graphics.reduceClipRegion(surround);

    if (! graphics.isClipEmpty()) {
        lookAndFeel.drawFrostedGlass(graphics, *this, nodeCanvas, graphics.getClipBounds());
    }

    graphics.restoreState();

    graphics.setColour(lookAndFeel.borderColour);
    graphics.drawRoundedRectangle(frame, ArsNovaUI::Theme::canvasCornerRadius, ArsNovaUI::Theme::borderThickness);
}
